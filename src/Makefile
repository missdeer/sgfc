# Makefile for SGFC
# Copyright (C) 1996-2021 by Arno Hollosi
# (see 'main.c' for more copyright information)

# System configuration
# SHELL = /bin/bash

# System environment
CC = gcc
#CC = /home/arno/development/sgfc/AFLplusplus-4.08c/afl-cc

OPTIONS = -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-implicit-fallthrough

# clang
#CC = clang
#OPTIONS = -std=c99 -Weverything -Wno-padded -Wno-unused-parameter \
		  -Wno-format-nonliteral -Wno-disabled-macro-expansion \
		  -Wno-documentation-unknown-command -Wno-switch-enum \
		  -Wno-conditional-uninitialized -Wno-implicit-fallthrough \
		  -Wno-reserved-identifier -Wno-missing-noreturn -Wno-string-concatenation

OPTIMIZATION = -O1
CFLAGS = $(OPTIMIZATION) $(OPTIONS)

LIB = -lm
OBJ = execute.o gameinfo.o load.o main.o parse.o parse2.o options.o\
	properties.o save.o strict.o util.o error.o encoding.o

sgfc: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LIB)

all: clean sgfc

clean:
	rm -f $(OBJ) sgfc

%.o: %.c all.h protos.h 
	$(CC) $(CFLAGS) -c $<
