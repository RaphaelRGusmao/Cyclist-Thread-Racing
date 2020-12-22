################################################################################
#                                IME-USP (2017)                                #
#                    MAC0422 - Sistemas Operacionais - EP2                     #
#                                                                              #
#                                   Makefile                                   #
#                                                                              #
#                              Raphael R. Gusmao                               #
################################################################################

.PHONY: clean
CC = gcc
CFLAGS = -Wall -O2 -g
OBJS = \
	error_handler.o \
	cyclist.o \
	pista.o \
	ep2.o

all: ep2

ep2: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread
	make clean

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *~

################################################################################
