/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                                   Pista                                    *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#ifndef __PISTA_H__
#define __PISTA_H__

#include <pthread.h>
#include "cyclist.h"

// Cores
#define CYAN    "\033[36;1m" // Azul claro
#define BLUE    "\033[34;1m" // Azul
#define GREEN   "\033[32;1m" // Verde
#define PINK    "\033[35;1m" // Rosa
#define YELLOW  "\033[33;1m" // Amarelo
#define END     "\033[0m"    // Para de pintar

// Inicializa a pista
void PISTA_init (int lanes, int d, int n, int v, int debug);

// Libera toda a memoria alocada
void PISTA_free_all ();

// Converte milissegundos para o formato "...h --m --s ---ms"
char *PISTA_format_time (int cur_time);

// Exibe os resultados finais da corrida
void PISTA_print_results ();

// Exibe a pista (debug)
void PISTA_show ();

// Exibe o ranking dos ciclistas na volta lap
void PISTA_print_ranking (int lap);

// Armazena no buffer que o ciclista id completou a volta lap
// Devolve o rank do ciclista na volta (colocacao por posicao)
int PISTA_set_rank_buffer (int lap, int id);

// Devolve o id do ciclista que chegou na posicao rank na volta lap
// Devolve 0 se ninguem chegou nessa posicao ainda
int PISTA_get_id_in_rank_buffer (int lap, int rank);

// Exibe o ranking da ultima volta completada
// Pode acontecer um caso em que quase todos os ciclistas terminaram  a  corrida
// mas falta um que ainda tem algumas voltas para completar.  Porem  ele  quebra
// antes de chegar. Assim a  corrida  acaba  "antes  da  hora",  pois  todos  os
// ciclistas pararam de correr. Entao essa funcao deve  exibir  os  rankings  de
// todas as voltas que restaram.
void PISTA_print_rankings_if_ready ();

// Devolve a posicao do ciclista id na colocacao por pontos do ultimo sprint
int PISTA_get_score_position (int id);

// Devolve a ultima volta em que a colocacao por pontos foi calculada
int PISTA_get_last_sprint ();

// Coloca o ciclista id na posicao pos e faixa lane
void PISTA_set_position (int lane, int pos, int id);

// Devolve o id do ciclista que esta nesssa posicao da pista
// Devolve 0 se a posicao esta vazia
int PISTA_get_id_in_position (int lane, int pos);

// Diminui em 1 o numero atual de ciclistas na corrida
// lap eh a volta em que o ciclista saiu
void PISTA_drop_n_cyclists_cur (int lap);

// Diminui em 1 o numero de dummies
void PISTA_drop_dummy ();

// Define a corrida como finalizada
void PISTA_set_game_over ();

// Define o intervalo de tempo entre cada loop (ms)
void PISTA_set_delta_time (int dt);

/******************************************************************************/

// Define o vetor de ciclistas
void PISTA_set_cyclists (Cyclist *cycs);

// Define as barreiras de sincronizacao de movimento e de tempo
void PISTA_set_barriers (pthread_barrier_t mbar, pthread_barrier_t tbar);

// Define o vetor de semaforos
void PISTA_set_mutexes (pthread_mutex_t *muts);

// Define os semaforos para mudanca de posicao
void PISTA_set_pos_mutexes (pthread_mutex_t plm, pthread_mutex_t pum);

/******************************************************************************/

// Devolve o numero de faixas da pista
int PISTA_get_lanes ();

// Devolve o comprimento da pista
int PISTA_get_length ();

// Devolve o numero inicial de ciclistas
int PISTA_get_n_cyclists ();

// Devolve o numero atual de ciclistas
int PISTA_get_n_cyclists_cur ();

// Devolve o numero de voltas
int PISTA_get_laps ();

// Devolve a ultima volta que foi exibido o ranking
int PISTA_get_last_rank_printed ();

// Devolve se o modo debug esta ativo ou nao
int PISTA_has_debug ();

// Devolve o numero de dummies em execucao
int PISTA_get_n_dummies ();

// Devolve se a corrida acabou ou nao
int PISTA_is_game_over ();

// Devolve o intervalo de tempo entre cada loop (ms)
int PISTA_get_delta_time ();

// Devolve o ciclista id
Cyclist PISTA_get_cyclist (int id);

// Barreira de sincronizacao de movimento
int PISTA_wait_move_barrier ();

// Barreira de sincronizacao de tempo
int PISTA_wait_time_barrier ();

// Tranca 2 linhas/posicoes consecutivas da pista
void PISTA_lock_position (int cur_pos, int next_pos);

// Destranca 2 linhas/posicoes consecutivas da pista
void PISTA_unlock_position (int cur_pos, int next_pos);

#endif

/******************************************************************************/
