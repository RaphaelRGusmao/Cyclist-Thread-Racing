/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                               Error handler                                *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#ifndef __ERROR_HANDLER__
#define __ERROR_HANDLER__

// Exibe uma mensagem de erro e finaliza o programa
void die_with_msg (const char* expression, ...);

// Aloca memoria (finaliza o programa caso nao tenha memoria suficiente)
void* emalloc (size_t size);

#endif

/******************************************************************************/
