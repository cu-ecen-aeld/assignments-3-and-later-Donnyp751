# Simple Makefile for cross compiling writer program
# Donald Posterick

CC=gcc
LD=ld
ARGS= -g



all: writer

writer: writer.c
	$(CROSS_COMPILE)$(CC) $(ARGS) $< -o $@


clean:
	rm -f writer
