/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                                  Ciclista                                  *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#ifndef __CYCLIST_H__
#define __CYCLIST_H__

// Ciclista
struct cyclist {
    int id;            // Identificacao         [1, n]
    int rank;          // Colocacao por posicao [1, n]
    int speed;         // Velocidade em km/h    {30, 60, 90}
    int pos;           // Posicao na pista      [0, d-1]
    int lap;           // Volta                 [0, v]
    int lane;          // Faixa          externa[0, 9]interna
    int original_lane; // Faixa que o ciclista comecou a corrida
    int pedaled;       // {1: ja pedalou na rodada, 0: ainda nao pedalou}
    int score;         // Pontuacao
    int is_broken;     // Esta quebrado?
    int is_lucky;      // Esta com sorte para pedalar a 90km/h nas 2 ultimas voltas?
    int cur_time;      // Instante de tempo em milissegundos em que o ciclista esta
};
typedef struct cyclist *Cyclist;

// Direcoes
#define LEFT   -1 // Para a esquerda (+ externa)
#define FORWARD 0 // Para frente
#define RIGHT   1 // Para a direita (+ interna)

// Cria um ciclista
// Todos comecam atras da linha de chegada com 30 km/h
Cyclist CYCLIST_new (int id);

// Funcao de comparacao de ciclistas com base em suas pontuacoes (qsort)
int CYCLIST_comp_score (const void *p_cyclist1, const void *p_cyclist2);

// Remove o ciclista da pista
// e cria uma thread dummy para ficar no lugar da thread do ciclista
// (Para as barreiras funcionarem corretamente)
void CYCLIST_remove (Cyclist cyclist);

// Move o ciclista 1 unidade na direcao direction
// Considera que a nova posicao esta livre, entao eh preciso verificar antes
// Se o ciclista completou uma volta, devolve sua colocacao por posicao na volta
// Caso contrario, devolve 0
int CYCLIST_step (Cyclist cyclist, int pos, int direction);

// Sincroniza as Threads (movimentos do ciclistas)
void CYCLIST_move_sync ();

// Sincroniza as Threads (tempo)
void CYCLIST_time_sync (int cur_time);

// Thread dummy para ficar no lugar dos ciclistas que forem removidos
void *CYCLIST_dummy (void *p_cur_time);

// Thread do ciclista apontado por p_cyclist
void *CYCLIST_thread (void *p_cyclist);

#endif

/******************************************************************************/
