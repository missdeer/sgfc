# Makefile for SGFC
# Copyright (C) 1996-2026 by Arno Hollosi
# (see 'COPYING' for more copyright information)

CC := gcc

ifeq ($(CC), gcc)
	WARNFLAGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
endif

ifeq ($(CC), clang)
	WARNFLAGS := -Weverything -Wno-padded -Wno-unused-parameter \
			  -Wno-documentation-unknown-command -Wno-declaration-after-statement \
			  -Wno-format-nonliteral -Wno-disabled-macro-expansion \
			  -Wno-switch-enum -Wno-unsafe-buffer-usage -Wno-switch-default \
			  -Wno-missing-prototypes
endif

OPTFLAGS := -O1
STD      := -std=c99
CPPFLAGS := -I../src -D_POSIX_C_SOURCE=200112L
CFLAGS   := $(OPTFLAGS) $(STD) $(WARNFLAGS) $(BUILD_CFLAGS) $(USER_CFLAGS)
LDFLAGS  :=
LDLIBS   := -lcheck -lpthread -lrt -lsubunit -lm

OBJ := test-runner.o test-helper.o position.o parse-text.o check-value.o \
	trigger-errors.o test-files.o load-properties.o check-encoding.o parse-move.o delete-node.o \
	value-length.o other-games.o check-options.o saving.o gameinfo-interactive.o nesting.o \
	gameinfo-correction.o input-size.o

SRC_OBJ := ../src/execute.o ../src/gameinfo.o ../src/load.o ../src/parse.o ../src/parse2.o \
	../src/options.o ../src/save.o ../src/properties.o ../src/strict.o ../src/util.o \
	../src/error.o ../src/encoding.o

RUN_PREFIX ?=

tests: sgfc-test
	$(RUN_PREFIX) ./sgfc-test

sgfc-test: $(OBJ) $(SRC_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c test-common.h ../src/all.h ../src/protos.h ../src/helpers.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) sgfc-test
	rm -f *.gcda *.gcno *.gcov

all: clean tests

.PHONY: all tests clean
