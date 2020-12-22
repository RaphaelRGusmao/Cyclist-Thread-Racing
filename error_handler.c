/******************************************************************************
 *                               IME-USP (2017)                               *
 *                   MAC0422 - Sistemas Operacionais - EP2                    *
 *                                                                            *
 *                               Error handler                                *
 *                                                                            *
 *                             Raphael R. Gusmao                              *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error_handler.h"

/******************************************************************************/
void die_with_msg (const char* expression, ...)
{
    char error_msg[1024];
    va_list args;
    va_start(args, expression);
    vsnprintf(error_msg, 1024, expression, args);
    va_end(args);
    printf("During the program, a fatal error ocurred:\n");
    fprintf(stderr, "\t%s\n", error_msg);
    exit(-1);
}

/******************************************************************************/
void* emalloc (size_t size)
{
    void *alloc = malloc(size);
    if (alloc == NULL)
        die_with_msg("Call to emalloc failed: Out of memory");
    return alloc;
}

/******************************************************************************/
