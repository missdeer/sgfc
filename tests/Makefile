# Makefile for SGFC
# Copyright (C) 1996-2021 by Arno Hollosi
# (see 'COPYING' for more copyright information)

# System configuration
# SHELL = /bin/bash

# System environment
CC = gcc

OPTIONS = -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter

DIRECTORIES = -I ../src
OPTIMIZATION = -O1
CFLAGS = $(DIRECTORIES) $(OPTIMIZATION) $(OPTIONS)

LIB = -lcheck -lpthread -lrt -lsubunit -lm
OBJ = test-runner.o test-helper.o position.o parse-text.o check-value.o\
	trigger-errors.o test-files.o load-properties.o encoding.o delete-node.o\
	value-length.o other-games.o options.o

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
