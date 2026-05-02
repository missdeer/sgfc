# Makefile for SGFC
# Copyright (C) 1996-2026 by Arno Hollosi
# (see 'COPYING' for more copyright information)

CC := gcc
#CC := /home/arno/development/sgfc/AFLplusplus-4.35c/afl-cc

OPTFLAGS  ?= -O1
STD       := -std=c99
WARNFLAGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter

CPPFLAGS +=
CFLAGS   += $(OPTFLAGS) $(STD) $(WARNFLAGS) $(COVERAGEFLAGS)
LDFLAGS  +=
LDLIBS   += -lm

# clang
#CC := clang
#WARNFLAGS := -Weverything -Wno-padded -Wno-unused-parameter \
		  -Wno-documentation-unknown-command -Wno-declaration-after-statement \
		  -Wno-format-nonliteral -Wno-disabled-macro-expansion \
		  -Wno-switch-enum

OBJ := execute.o gameinfo.o load.o main.o parse.o parse2.o options.o\
	properties.o save.o strict.o util.o error.o encoding.o


sgfc: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

clean:
	rm -f $(OBJ) sgfc
	rm -f *.gcda *.gcno *.gcov

%.o: %.c all.h protos.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

all: clean sgfc

.PHONY: all clean
