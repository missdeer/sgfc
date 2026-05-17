/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/test-common.h
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include "all.h"
#include "protos.h"

/* test-helper.c */

extern struct SGFInfo *sgfc;
extern char *expected_output;
extern uint32_t expected_error;
extern uint32_t *allowed_errors;
extern bool expected_error_occurred;

struct SaveFileHandler *SetupSaveTestIO(void);
bool verifying_error_handler(uint32_t type, struct SGFInfo *sgfi, va_list arglist);
void common_setup(void);
void common_teardown(void);

#endif /* TEST_COMMON_H_ */
