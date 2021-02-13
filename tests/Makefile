# Makefile for SGFC
# Copyright (C) 1996-2021 by Arno Hollosi
# (see 'COPYING' for more copyright information)

# System configuration
# SHELL = /bin/bash

# System environment
CC = gcc

OPTIONS = -std=c99 -pedantic -W -Wimplicit -Wreturn-type -Wswitch -Wformat\
	  -Wuninitialized -Wparentheses -Wpointer-arith -Wbad-function-cast\
	  -Wcast-qual -Wcast-align -Waggregate-return -Wstrict-prototypes\
	  -Wnested-externs -Wshadow -Wchar-subscripts -Wno-unused-parameter\
	  -Wno-implicit-fallthrough -Wconversion -Wsign-conversion

DIRECTORIES = -I ../src
OPTIMIZATION = -O1
CFLAGS = $(DIRECTORIES) $(OPTIMIZATION) $(OPTIONS)

LIB = -lcheck -lpthread -lrt -lsubunit -lm
OBJ = test-runner.o test-helper.o position.o parse-text.o check-value.o\
	trigger-errors.o test-files.o load-properties.o encoding.o delete-node.o\
	value-length.o

SRC_OBJ = ../src/execute.o ../src/gameinfo.o ../src/load.o\
	../src/parse.o ../src/parse2.o ../src/options.o ../src/save.o\
	../src/properties.o ../src/strict.o ../src/util.o ../src/error.o\
	../src/encoding.o

sgfc-test: $(OBJ) $(SRC_OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(SRC_OBJ) -o $@ $(LIB)

tests: sgfc-test
	./sgfc-test

all: clean tests

clean:
	rm -f $(OBJ) sgfc-test

%.o: %.c test-common.h ../src/all.h ../src/protos.h
	$(CC) $(CFLAGS) -c $<
