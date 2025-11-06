# Simple Makefile for cross compiling socket program
# Donald Posterick

CC=gcc
LD=ld
ARGS= -g



all: aesdsocket

aesdsocket: aesdsocket.c linkedlist.c
	$(CROSS_COMPILE)$(CC) $(ARGS) $^ -o $@


clean:
	rm -f aesdsocket
