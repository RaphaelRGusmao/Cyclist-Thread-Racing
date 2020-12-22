/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                                 Principal                                  *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "pista.h"
#include "cyclist.h"
#include "error_handler.h"

#define LANES 10

/******************************************************************************/
// Simula a corrida
void simulate (int d, int n, int v, int debug)
{
    srand(time(NULL));

    // Inicializa a pista e cria os ciclistas
    PISTA_init(LANES, d, n, v, debug);
    Cyclist *cyclists = emalloc((n + 1) * sizeof(Cyclist));
    for (int i = 1; i <= n; i++) {
        cyclists[i] = CYCLIST_new(i);
        PISTA_set_position(cyclists[i]->lane, cyclists[i]->pos, cyclists[i]->id);
    }

    // 10% de chance de um ciclista aleatorio ter sorte de pedalar a 90km/h no final
    if (rand() < 0.1*(RAND_MAX + 1.0))
        cyclists[(rand() % n) + 1]->is_lucky = 1;
    PISTA_set_cyclists(cyclists);

    // Cria a barreira de sincronizacao de movimento e de tempo para os n ciclistas
    pthread_barrier_t move_barrier, time_barrier;
    pthread_barrier_init(&move_barrier, NULL, n);
    pthread_barrier_init(&time_barrier, NULL, n);
    PISTA_set_barriers(move_barrier, time_barrier);

    // Cria os semaforos, um para cada linha/posicao, e mais 2 para mudanca de posicao
    pthread_mutex_t *mutexes = emalloc(d * sizeof(pthread_mutex_t));
    for (int i = 0; i < d; i++) {
        if (pthread_mutex_init(&mutexes[i], NULL))
            die_with_msg("Error initializing mutex %d", i);
    }
    PISTA_set_mutexes(mutexes);
    pthread_mutex_t pos_lock_mutex, pos_unlock_mutex;
    if (pthread_mutex_init(&pos_lock_mutex, NULL))
        die_with_msg("Error initializing pos_lock_mutex");
    if (pthread_mutex_init(&pos_unlock_mutex, NULL))
        die_with_msg("Error initializing pos_unlock_mutex");
    PISTA_set_pos_mutexes(pos_lock_mutex, pos_unlock_mutex);

    printf(CYAN "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[ Inicio da corrida ]\n\n" END);

    if (debug) PISTA_show();

    // Cria as threads
    pthread_t *threads = emalloc((n + 1) * sizeof(pthread_t));
    for (int i = 1; i <= n; i++) {
        if (pthread_create(&threads[i], NULL, CYCLIST_thread, &cyclists[i]))
            die_with_msg("Error creating thread %d", i);
    }

    // Espera as threads dos ciclistas e as dummies terminarem de executar
    for (int i = 1; i <= n; i++) {
        if (pthread_join(threads[i], NULL))
            die_with_msg("Error joining thread %d", i);
    }
    while (PISTA_get_n_dummies() > 0);

    printf(CYAN "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[ Fim da corrida ]\n\n" END);

    // Exibe os resultados finais da corrida
    PISTA_print_results();

    // Libera toda a memoria alocada
    PISTA_free_all();
    free(threads);
}

/******************************************************************************/
// Funcao principal
int main (int argc, char **argv)
{
    if (argc != 4 && argc != 5)
        die_with_msg("Usage: ./ep2 <velodrome_length> <number_of_cyclists> <number_of_laps> [debug]");

    int d = atoi(argv[1]);
    int n = atoi(argv[2]);
    int v = atoi(argv[3]);
    int debug = 0;
    if (argc == 5) {
        if (!strcmp(argv[4], "debug"))
            debug = 1;
        else
            die_with_msg("Invalid entry");
    }

    simulate(d, n, v, debug);

    return 0;
}

/******************************************************************************/
