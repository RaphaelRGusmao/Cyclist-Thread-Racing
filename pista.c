/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                                   Pista                                    *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "pista.h"
#include "cyclist.h"
#include "error_handler.h"

// Variaveis globais
static int **pista;                      // Matriz da pista
static int **rank_buffer;                // Armazena as colocacoes em cada volta
static int *n_cyclists_in_lap;           // Numero de ciclistas que completarao cada volta
static int pista_lanes;                  // Quantidade de faixas
static int pista_length;                 // Comprimento da pista
static int n_cyclists;                   // Quantidade inicial de ciclistas
static int n_cyclists_cur;               // Quantidade atual de ciclistas
static int n_cyclist_finished;           // Quantos terminaram sem quebrar
static int laps;                         // Quantidade de voltas
static int last_rank_printed;            // Ultimo ranking exibido
static int last_sprint;                  // Ultima volta em que a colocacao por pontos foi calculada
static int debug;                        // Opcao de debug {1:on, 0:off}
static int n_dummies;                    // Quantidade de dummies
static int game_over;                    // A corrida acabou? {1:sim, 0:nao}
static int delta_time;                   // Intervalo de tempo entre cada loop (ms)
static Cyclist *cyclists;                // Vetor de ciclistas
static Cyclist **cyclists_by_score;      // Vetor de ciclistas ordenado por score
static pthread_barrier_t move_barrier;   // Barreira de sincronizacao de movimento
static pthread_barrier_t time_barrier;   // Barreira de sincronizacao de tempo
static pthread_mutex_t *mutexes;         // Vetor de semaforos
static pthread_mutex_t pos_lock_mutex;   // Semaforo para trancar uma posicao
static pthread_mutex_t pos_unlock_mutex; // Semaforo para destrancar uma posicao

/******************************************************************************/
void PISTA_init (int lanes, int d, int n, int v, int dbg)
{
    pista_lanes = lanes;
    pista_length = d;
    n_cyclists = n_cyclists_cur = n;
    laps = v;
    debug = dbg;
    n_dummies = 0;
    game_over = 0;
    delta_time = 60;
    pista = emalloc(pista_lanes * sizeof(int*));
    for (int i = 0; i < pista_lanes; i++) {
        pista[i] = emalloc(d * sizeof(int));
        for (int j = 0; j < d; j++) {
            pista[i][j] = 0;
        }
    }
    rank_buffer = emalloc((v + 1) * sizeof(int*));
    last_rank_printed = 0;
    last_sprint = 0;
    n_cyclists_in_lap = emalloc((v + 1) * sizeof(int));
    for (int i = 1; i <= v; i++) {
        n_cyclists_in_lap[i] = n;
    }
}

/******************************************************************************/
void PISTA_free_all ()
{
    for (int i = 0; i < pista_lanes; i++) free(pista[i]);
    for (int i = 1; i <= n_cyclists; i++) free(cyclists[i]);
    free(pista);
    free(cyclists);
    free(cyclists_by_score);
    free(n_cyclists_in_lap);
    free(rank_buffer);
    free(mutexes);
    pthread_barrier_destroy(&move_barrier);
    pthread_barrier_destroy(&time_barrier);
    pthread_mutex_destroy(&pos_lock_mutex);
    pthread_mutex_destroy(&pos_unlock_mutex);
}

/******************************************************************************/
char *PISTA_format_time (int cur_time)
{
    int ms  =  cur_time % 1000;
    int s   = (cur_time / 1000) % 60;
    int min = (cur_time / (1000*60)) % 60;
    int h   = (cur_time / (1000*60*60)) % 24;
    char *ret = emalloc(20 * sizeof(char));
    snprintf(ret, 20, "%dh %02dm %02ds %03dms", h, min, s, ms);
    return ret;
}

/******************************************************************************/
void PISTA_print_results ()
{
    printf("+------------------------------------------------------------------------------+\n");
    printf("| ");
    printf(CYAN "Resultado final" END);
    printf("                                                              |\n");
    printf("|-----------------------+---------------+-------------------+------------------|\n");
    printf("| Colocacao             | Ciclista      | Pontos            | Tempo de corrida |\n");
    printf("+-----------------------+---------------+-------------------+------------------+\n");
    for (int i = 1; i <= n_cyclists; i++) {
        Cyclist cyc = *(cyclists_by_score[i]);
        char *race_time = PISTA_format_time(cyc->cur_time);
        if (cyc->is_broken) {
            printf("| Quebrou na volta %4d | %4d\t\t| %4d\t\t    | %s |\n",
                cyc->lap, cyc->id, cyc->score, race_time);
        } else {
            printf("| %4d\t\t\t| %4d\t\t| %4d\t\t    | %s |\n",
                i, cyc->id, cyc->score, race_time);
        }
    }
    printf("+-----------------------+---------------+-------------------+------------------+\n\n");
}

/******************************************************************************/
void PISTA_show ()
{
    printf("    ");
    for (int i = 0; i < pista_lanes; i++) printf("%5d ", i);
    printf("\n");
    printf("     ^");
    for (int i = 0; i <= 6*pista_lanes-2; i++) printf("=");
    printf("^\n");
    for (int j = pista_length-1; j >= 0; j--) {
        printf("%4d |", j);
        for (int i = 0; i < pista_lanes; i++) {
            int id = pista[i][j];
            if (id != 0) {
                if (cyclists[id]->speed == 30) {
                    printf(BLUE "%5d" END, pista[i][j]);
                } else if (cyclists[id]->speed == 60) {
                    printf(GREEN "%5d" END, pista[i][j]);
                } else {
                    printf(PINK "%5d" END, pista[i][j]);
                }
            } else printf("  -  ");
            printf("|");
        }
        printf("\n");
    }
    printf("\n");
}

/******************************************************************************/
void PISTA_print_ranking (int lap)
{
    if (lap % 10 == 0) {// Voltas multiplas de 10
        printf("+------------------------------------------------------------------------------+\n");
        printf("| ");
        printf(CYAN "Volta %-4d - Sprint" END, lap);
        printf("                                                          |\n");
        printf("|-------------+------------------------------------+----------------+----------|\n");
        printf("| Colocacao   | Ciclista       |      | Colocacao  | Ciclista       | Pontos   |\n");
        printf("| por posicao |                |      | por pontos |                |          |\n");
        printf("+-------------+----------------+      +------------+----------------+----------+\n");
        int i;
        for (i = 1; i <= rank_buffer[lap][0]; i++) {
            int id_rank = rank_buffer[lap][i];
            int id_score = (*(cyclists_by_score[i]))->id;
            printf("| %5d lugar |", i);
            if (cyclists[id_rank]->is_broken && cyclists[id_rank]->lap == lap)
                printf(YELLOW " %4d (quebrou) " END, id_rank);
            else
                printf(" %4d           ", id_rank);
            printf("|      |");
            if (cyclists[id_score]->is_broken) {
                if (cyclists[id_score]->lap == lap) {
                    printf(" %4d lugar |", i);
                    printf(YELLOW " %4d (quebrou) " END, id_score);
                    printf("| %4d     |\n", cyclists[id_score]->score);
                } else {
                    printf("    -       |    -           |    -     |\n");
                }
            } else {
                printf(" %4d lugar |", i);
                printf(" %4d\t    ", id_score);
                printf("| %4d     |\n", cyclists[id_score]->score);
            }
        }
        for (; i <= n_cyclists; i++) {
            printf("|     -       |    -           |      |    -       |    -           |    -     |\n");
        }
        printf("+-------------+----------------+      +------------+----------------+----------+\n\n");
    } else {// Voltas nao multiplas de 10
        printf("+------------------------------+\n");
        printf("| ");
        printf(CYAN "Volta %-4d" END, lap);
        printf("                   |\n");
        printf("|-------------+----------------|\n");
        printf("| Colocacao   | Ciclista       |\n");
        printf("| por posicao |                |\n");
        printf("+-------------+----------------+\n");
        int i;
        for (i = 1; i <= rank_buffer[lap][0]; i++) {
            int id_rank = rank_buffer[lap][i];
            printf("| %5d lugar |", i);
            if (cyclists[id_rank]->is_broken && cyclists[id_rank]->lap == lap) {
                int id_score = 0;
                while ((*(cyclists_by_score[++id_score]))->id != id_rank);
                printf(YELLOW " %4d (quebrou)" END, id_rank);
                printf(" | ");
                printf(YELLOW "<- %d lugar na colocacao por pontos\n" END, id_score);
            } else {
                printf(" %4d           |\n", id_rank);
            }
        }
        for (; i <= n_cyclists; i++) {
            printf("|     -       |    -           |\n");
        }
        printf("+-------------+----------------+\n\n");
    }
    last_rank_printed++;
    free(rank_buffer[lap]);
}

/******************************************************************************/
int PISTA_set_rank_buffer (int lap, int id)
{
    if (!rank_buffer[lap]) {
        rank_buffer[lap] = emalloc((n_cyclists_in_lap[lap] + 1) * sizeof(int));
        rank_buffer[lap][0] = 0;
    }
    int rank = ++rank_buffer[lap][0];
    rank_buffer[lap][rank] = id;
    return rank;
}

/******************************************************************************/
int PISTA_get_id_in_rank_buffer (int lap, int rank)
{
    if (!rank_buffer[lap]) return -1;
    return rank_buffer[lap][rank];
}

/******************************************************************************/
void PISTA_print_rankings_if_ready ()
{
    while (1) {
        int lap = last_rank_printed + 1;
        if (lap > laps || !rank_buffer[lap]) break;
        int n_in_buffer = rank_buffer[lap][0];
        if (n_in_buffer == n_cyclists_in_lap[lap]) {
            if (lap % 10 == 0) {// Voltas multiplas de 10
                cyclists[rank_buffer[lap][1]]->score += 5;// 1 lugar ganha 5 pontos
                cyclists[rank_buffer[lap][2]]->score += 3;// 2 lugar ganha 3 pontos
                cyclists[rank_buffer[lap][3]]->score += 2;// 3 lugar ganha 2 pontos
                cyclists[rank_buffer[lap][4]]->score += 1;// 4 lugar ganha 1 pontos
                for (int i = 1; i <= n_in_buffer; i++) {
                    cyclists[rank_buffer[lap][i]]->rank = i;
                }
                qsort(cyclists_by_score+1, n_cyclists, sizeof(Cyclist*), CYCLIST_comp_score);
                last_sprint = lap;
            }
            PISTA_print_ranking(lap);
        } else break;
    }
}

/******************************************************************************/
int PISTA_get_score_position (int id)
{
    for (int i = 1; i <= n_cyclists; i++) {
        if ((*(cyclists_by_score[i]))->id == id)
            return i;
    }
    return 0;
}

/******************************************************************************/
int PISTA_get_last_sprint ()
{
    return last_sprint;
}

/******************************************************************************/
void PISTA_set_position (int lane, int pos, int id)
{
    pista[lane][pos] = id;
}

/******************************************************************************/
int PISTA_get_id_in_position (int lane, int pos)
{
    return pista[lane][pos];
}

/******************************************************************************/
void PISTA_drop_n_cyclists_cur (int lap)
{
    n_cyclists_cur--;
    n_dummies++;
    if (lap == laps) {
        n_cyclist_finished++;
    } else {// Quebrou
        // O ciclista nao estara mais nas voltas seguintes
        for (int i = lap + 1; i <= laps; i++) {
            n_cyclists_in_lap[i]--;
        }
    }
}

/******************************************************************************/
void PISTA_drop_dummy ()
{
    n_dummies--;
}

/******************************************************************************/
void PISTA_set_game_over ()
{
    game_over = 1;
}

/******************************************************************************/
void PISTA_set_delta_time (int dt)
{
    delta_time = dt;
}

/******************************************************************************/
void PISTA_set_cyclists (Cyclist *cycs)
{
    cyclists = cycs;
    cyclists_by_score = emalloc((n_cyclists + 1) * sizeof(Cyclist*));
    for (int i = 1; i <= n_cyclists; i++) {
        cyclists_by_score[i] = &cyclists[i];
    }
}

/******************************************************************************/
void PISTA_set_barriers (pthread_barrier_t mbar, pthread_barrier_t tbar)
{
    move_barrier = mbar;
    time_barrier = tbar;
}

/******************************************************************************/
void PISTA_set_mutexes (pthread_mutex_t *muts)
{
    mutexes = muts;
}

/******************************************************************************/
void PISTA_set_pos_mutexes (pthread_mutex_t plm, pthread_mutex_t pum)
{
    pos_lock_mutex = plm;
    pos_unlock_mutex = pum;
}

/******************************************************************************/
int PISTA_get_lanes ()
{
    return pista_lanes;
}

/******************************************************************************/
int PISTA_get_length ()
{
    return pista_length;
}

/******************************************************************************/
int PISTA_get_n_cyclists ()
{
    return n_cyclists;
}

/******************************************************************************/
int PISTA_get_n_cyclists_cur ()
{
    return n_cyclists_cur;
}

/******************************************************************************/
int PISTA_get_laps ()
{
    return laps;
}

/******************************************************************************/
int PISTA_get_last_rank_printed ()
{
    return last_rank_printed;
}

/******************************************************************************/
int PISTA_has_debug ()
{
    return debug;
}

/******************************************************************************/
int PISTA_get_n_dummies ()
{
    return n_dummies;
}

/******************************************************************************/
int PISTA_is_game_over ()
{
    return game_over;
}

/******************************************************************************/
int PISTA_get_delta_time ()
{
    return delta_time;
}

/******************************************************************************/
Cyclist PISTA_get_cyclist (int id)
{
    return cyclists[id];
}

/******************************************************************************/
int PISTA_wait_move_barrier ()
{
    return pthread_barrier_wait(&move_barrier);
}

/******************************************************************************/
int PISTA_wait_time_barrier ()
{
    return pthread_barrier_wait(&time_barrier);
}

/******************************************************************************/
void PISTA_lock_position (int cur_pos, int next_pos)
{
    pthread_mutex_lock(&pos_lock_mutex);
    pthread_mutex_lock(mutexes + cur_pos);
    pthread_mutex_lock(mutexes + next_pos);
    pthread_mutex_unlock(&pos_lock_mutex);
}

/******************************************************************************/
void PISTA_unlock_position (int cur_pos, int next_pos)
{
    pthread_mutex_lock(&pos_unlock_mutex);
    pthread_mutex_unlock(mutexes + cur_pos);
    pthread_mutex_unlock(mutexes + next_pos);
    pthread_mutex_unlock(&pos_unlock_mutex);
}

/******************************************************************************/
