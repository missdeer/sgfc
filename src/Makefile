# Makefile for SGFC
# Copyright (C) 1996-2026 by Arno Hollosi
# (see 'COPYING' for more copyright information)

CC := gcc
#CC := /home/arno/development/sgfc/AFLplusplus-4.35c/afl-cc

ifeq ($(CC), gcc)
	WARNFLAGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
endif

ifeq ($(CC), clang)
	WARNFLAGS := -Weverything -Wno-padded -Wno-unused-parameter \
			  -Wno-documentation-unknown-command -Wno-declaration-after-statement \
			  -Wno-format-nonliteral -Wno-disabled-macro-expansion \
			  -Wno-switch-enum -Wno-unsafe-buffer-usage -Wno-switch-default
endif

OPTFLAGS := -O1
STD      := -std=c99
CPPFLAGS :=
CFLAGS   := $(OPTFLAGS) $(STD) $(WARNFLAGS) $(BUILD_CFLAGS) $(USER_CFLAGS)
LDFLAGS  :=
LDLIBS   := -lm

OBJ := execute.o gameinfo.o load.o main.o parse.o parse2.o options.o \
	properties.o save.o strict.o util.o error.o encoding.o


sgfc: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(OBJ) sgfc
	rm -f *.gcda *.gcno *.gcov

%.o: %.c all.h protos.h helpers.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

all: clean sgfc

.PHONY: all clean
