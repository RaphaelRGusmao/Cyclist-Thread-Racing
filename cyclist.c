/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                                  Ciclista                                  *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "pista.h"
#include "cyclist.h"
#include "error_handler.h"

/******************************************************************************/
Cyclist CYCLIST_new (int id)
{
    Cyclist cyclist = emalloc(sizeof(*cyclist));
    cyclist->id = id;
    cyclist->rank = id;
    cyclist->speed = 30;
    cyclist->pos = PISTA_get_length()-1 - (id-1)/PISTA_get_lanes();
    cyclist->lap = -1;
    cyclist->lane = cyclist->original_lane = (id-1) % PISTA_get_lanes();
    cyclist->pedaled = 0;
    cyclist->score = 0;
    cyclist->is_broken = 0;
    cyclist->is_lucky = 0;
    cyclist->cur_time = 0;
    return cyclist;
}

/******************************************************************************/
int CYCLIST_comp_score (const void *p_cyclist1, const void *p_cyclist2)
{
    // Se quebrou na volta que vai ser exibida, considera que nao quebrou ainda
    Cyclist cyclist1 = *(*((Cyclist**)p_cyclist1));
    Cyclist cyclist2 = *(*((Cyclist**)p_cyclist2));

    // Se apenas o cyclist 1 esta quebrado
    if (cyclist1->is_broken && !cyclist2->is_broken)
        if (cyclist1->lap != PISTA_get_last_rank_printed()+1)
            return  1;// (2 1)

    // Se apenas o cyclist 2 esta quebrado
    if (!cyclist1->is_broken && cyclist2->is_broken)
        if (cyclist2->lap != PISTA_get_last_rank_printed()+1)
            return -1;// (1 2)

    // Compara a volta que os ciclistas estao (os quebrados ficam para tras)
    if (cyclist1->lap > cyclist2->lap) return -1;// (1 2)
    if (cyclist1->lap < cyclist2->lap) return  1;// (2 1)

    // Compara a pontuacao
    if (cyclist1->score > cyclist2->score) return -1;// (1 2)
    if (cyclist1->score < cyclist2->score) return  1;// (2 1)

    // Compara a colocacao por posicao na volta que vai ser exibida
    if (cyclist1->rank > cyclist2->rank) return  1;// (2 1)
    if (cyclist1->rank < cyclist2->rank) return -1;// (1 2)

    return 0;
}

/******************************************************************************/
void CYCLIST_remove (Cyclist cyclist)
{
    PISTA_lock_position(PISTA_get_length()-1, 0);
    int id_in_position = PISTA_get_id_in_position(cyclist->lane, cyclist->pos);
    if (id_in_position == cyclist->id)
        PISTA_set_position(cyclist->lane, cyclist->pos, 0);
    PISTA_drop_n_cyclists_cur(cyclist->lap);
    pthread_t dummy;
    if (pthread_create(&dummy, NULL, CYCLIST_dummy, &cyclist->cur_time))
        die_with_msg("Error creating thread dummy");
    PISTA_unlock_position(PISTA_get_length()-1, 0);
}

/******************************************************************************/
int CYCLIST_step (Cyclist cyclist, int pos, int direction)
{
    int rank = 0;
    int id_in_position = PISTA_get_id_in_position(cyclist->lane, cyclist->pos);
    if (id_in_position == cyclist->id)
        PISTA_set_position(cyclist->lane, cyclist->pos, 0);
    cyclist->pos = pos;
    cyclist->lane += direction;
    if (pos == 0 && ++cyclist->lap > 0)// Completou uma volta
        rank = PISTA_set_rank_buffer(cyclist->lap, cyclist->id);
    cyclist->pedaled = 1;
    PISTA_set_position(cyclist->lane, cyclist->pos, cyclist->id);
    return rank;
}

/******************************************************************************/
void CYCLIST_move_sync ()
{
    // Somente a ultima thread a sincronizar entra no if
    if (PISTA_wait_move_barrier() == PTHREAD_BARRIER_SERIAL_THREAD) {
        for (int i = 1; i <= PISTA_get_n_cyclists(); i++)
            PISTA_get_cyclist(i)->pedaled = 0;
    }
    PISTA_wait_move_barrier();
}

/******************************************************************************/
void CYCLIST_time_sync (int cur_time)
{
    // Somente a ultima thread a sincronizar entra no if
    if (PISTA_wait_time_barrier() == PTHREAD_BARRIER_SERIAL_THREAD) {
        // Exibe a pista se estiver no modo debug
        if (PISTA_has_debug()) {
            char *race_time = PISTA_format_time(cur_time);
            printf(CYAN "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[ Tempo: %s ]\n" END, race_time);
            PISTA_show();
        }

        // Exibe o ranking caso alguma volta tenha sido completada por todos
        PISTA_print_rankings_if_ready();

        // Verifica se a corrida ja acabou
        if (PISTA_get_n_dummies() == PISTA_get_n_cyclists()) PISTA_set_game_over();
    }
    PISTA_wait_time_barrier();
}

/******************************************************************************/
void *CYCLIST_dummy (void *p_cur_time)
{
    int cur_time = *(int*)p_cur_time;
    while (1) {
        if (PISTA_is_game_over()) break;
        CYCLIST_time_sync(cur_time);
        cur_time += PISTA_get_delta_time();
        CYCLIST_move_sync();
    }
    PISTA_drop_dummy();
    return NULL;
}

/******************************************************************************/
void *CYCLIST_thread (void *p_cyclist)
{
    Cyclist cyclist = *((Cyclist*)p_cyclist);

    while (1) {
        cyclist->cur_time += PISTA_get_delta_time();

        // Verifica se esta na hora de pedalar (depende da velocidade)
        int rank = 0;
        if (cyclist->cur_time % (3600/cyclist->speed) == 0) {
            int cur_pos = cyclist->pos;
            int next_pos = (cyclist->pos + 1) % PISTA_get_length();

            PISTA_lock_position(cur_pos, next_pos);

            // Decide para onde o ciclista vai
            int id_front = PISTA_get_id_in_position(cyclist->lane, next_pos);
            if (!id_front) {// Ninguem na frente
                // Verifica se ja esta na faixa original
                if (cyclist->lane == cyclist->original_lane) {
                    // Vai para frente
                    rank = CYCLIST_step(cyclist, next_pos, FORWARD);
                } else {// Nao esta na faixa original
                    // Tenta voltar para a faixa original
                    int id_right = PISTA_get_id_in_position(cyclist->lane+RIGHT, next_pos);
                    if (!id_right) {// Ninguem na diagonal da direita
                        // Vai para a diagonal direita
                        rank = CYCLIST_step(cyclist, next_pos, RIGHT);
                    } else {// Tem um ciclista na diagonal da direita
                        // Verifica se o ciclista da diagonal direita ja vai sair
                        Cyclist cyc_right = PISTA_get_cyclist(id_right);
                        int is_cyc_right_time = cyclist->cur_time % (3600/cyc_right->speed) == 0;
                        if (is_cyc_right_time && !cyc_right->pedaled && cyclist->cur_time % 120 == 0) {
                            // Vai para a diagonal direita
                            rank = CYCLIST_step(cyclist, next_pos, RIGHT);
                        } else {// O ciclista da diagonal direita talvez nao vai sair agora
                            // Vai para frente
                            rank = CYCLIST_step(cyclist, next_pos, FORWARD);
                        }
                    }
                }
            } else {// Tem um ciclista na frente
                // Verifica se o ciclista da frente ja vai sair
                Cyclist cyc_front = PISTA_get_cyclist(id_front);
                int is_cyc_front_time = cyclist->cur_time % (3600/cyc_front->speed) == 0;
                if (is_cyc_front_time && !cyc_front->pedaled && cyclist->cur_time % 120 == 0) {
                    // Vai para frente
                    rank = CYCLIST_step(cyclist, next_pos, FORWARD);
                } else {// O ciclista da frente talvez nao vai sair agora
                    // Tenta ultrapassar pela esquerda (faixa mais externa)
                    if (cyclist->lane > 0) {// Tem faixa na esquerda
                        // Verifica se tem alguem na esquerda
                        int id_left = PISTA_get_id_in_position(cyclist->lane+LEFT, cur_pos);
                        if (!id_left) {// Ninguem na esquerda
                            int id_diag_left = PISTA_get_id_in_position(cyclist->lane+LEFT, next_pos);
                            if (!id_diag_left) {// Ninguem na diagonal esquerda
                                // Vai para a diagonal esquerda
                                rank = CYCLIST_step(cyclist, next_pos, LEFT);
                            } else {// Tem alguem na diagonal esquerda
                                // Verifica se o ciclista da diagonal esquerda ja vai sair
                                Cyclist cyc_diag_left = PISTA_get_cyclist(id_diag_left);
                                int is_cyc_diag_left_time = cyclist->cur_time % (3600/cyc_diag_left->speed) == 0;
                                if (is_cyc_diag_left_time && !cyc_diag_left->pedaled && cyclist->cur_time % 120 == 0) {
                                    // Vai para a diagonal esquerda
                                    rank = CYCLIST_step(cyclist, next_pos, LEFT);
                                } else {// O ciclista da diagonal esquerda talvez nao vai sair agora
                                    // Diminui a velocidade
                                    if (cyc_front->speed < cyclist->speed)
                                        cyclist->speed = cyc_front->speed;
                                }
                            }
                        } else {// Tem um ciclista na esquerda
                            // Diminui a velocidade
                            if (cyc_front->speed < cyclist->speed)
                                cyclist->speed = cyc_front->speed;
                        }
                    } else {// Nao tem faixa na esquerda
                        // Diminui a velocidade
                        if (cyc_front->speed < cyclist->speed)
                            cyclist->speed = cyc_front->speed;
                    }
                }
            }

            PISTA_unlock_position(cur_pos, next_pos);
        }

        // Espera todos os cyclists chegarem neste trecho para prosseguir
        CYCLIST_move_sync();

        // Completou uma volta
        if (rank > 0) {
            // Verifica se completou 1 volta sobre todos os outros
            if (rank == 1) {// Primeiro lugar na volta
                if (PISTA_get_id_in_rank_buffer(cyclist->lap-1, 2) == 0) {
                    // Ninguem chegou em segundo lugar na volta anterior ainda
                    if (PISTA_has_debug())
                        printf(YELLOW "* Ciclista %3d completou uma volta (%d) na frente em %s\n" END,
                            cyclist->id, cyclist->lap, PISTA_format_time(cyclist->cur_time));
                    cyclist->score += 20;
                }
            }

            // Completou a corrida?
            if (cyclist->lap == PISTA_get_laps()) {
                if (PISTA_has_debug())
                    printf(YELLOW "* Ciclista %3d acabou a corrida em %s\n" END,
                        cyclist->id, PISTA_format_time(cyclist->cur_time));
                break;
            }

            // Vai quebrar?
            if (cyclist->lap % 15 == 0 && PISTA_get_n_cyclists_cur() > 5) {
                if (rand() < 0.01*(RAND_MAX + 1.0)) {
                    printf(YELLOW "* Ciclista %3d " END, cyclist->id);
                    printf(YELLOW "quebrou na volta %d " END, cyclist->lap);
                    printf(YELLOW "em %s\n" END, PISTA_format_time(cyclist->cur_time));
                    printf(YELLOW "*    Posicao na colocacao por pontos no ultimo sprint (volta %d): %d\n" END,
                        PISTA_get_last_sprint(), PISTA_get_score_position(cyclist->id));
                    cyclist->is_broken = 1;
                    break;
                }
            }

            // Sorteia a nova velocidade
            if (cyclist->lap >= PISTA_get_laps()-2 && cyclist->is_lucky) {
                cyclist->speed = 90;
                PISTA_set_delta_time(20);
            } else if (cyclist->speed == 30) {
                if (rand() < 0.7*(RAND_MAX + 1.0)) {
                    cyclist->speed = 60;
                }
            } else if (cyclist->speed == 60) {
                if (rand() < 0.5*(RAND_MAX + 1.0)) {
                    cyclist->speed = 30;
                }
            }
        }

        // Sincronizar o tempo de todos os cyclists
        CYCLIST_time_sync(cyclist->cur_time);
    }

    // Remove o cyclist da pista
    CYCLIST_remove(cyclist);

    return NULL;
}

/******************************************************************************/
