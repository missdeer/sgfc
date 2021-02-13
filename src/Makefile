# Makefile for SGFC
# Copyright (C) 1996-2021 by Arno Hollosi
# (see 'main.c' for more copyright information)

# System configuration
# SHELL = /bin/bash

# System environment
CC = gcc
#CC = /home/arno/development/sgfc/afl-2.52b/afl-gcc

OPTIONS = -std=c99 -pedantic -W -Wimplicit -Wreturn-type -Wswitch -Wformat \
	  -Wuninitialized -Wparentheses -Wpointer-arith -Wbad-function-cast \
	  -Wcast-qual -Wcast-align -Waggregate-return -Wstrict-prototypes \
	  -Wnested-externs -Wshadow -Wchar-subscripts -Wno-unused-parameter \
	  -Wno-implicit-fallthrough -Wconversion -Wsign-conversion

# clang
#CC = clang
#OPTIONS = -std=c99 -Weverything -Wno-padded -Wno-unused-parameter \
		  -Wno-format-nonliteral -Wno-disabled-macro-expansion \
		  -Wno-documentation-unknown-command -Wno-switch-enum \
		  -Wno-conditional-uninitialized
#OPTIONS = -W -Wall -pedantic -std=c99 -Wno-unused-parameter -Wmicrosoft \
		  -Wsign-conversion -Wshorten-64-to-32 -Wconversion

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
