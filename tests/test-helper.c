/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/test-helper.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <check.h>

#include "test-common.h"


struct SGFInfo *sgfc;
char *expected_output;

uint32_t expected_error;
uint32_t *allowed_errors;		/* additional errors that might occur */
bool expected_error_occurred;


int Test_BufferIO_Close(struct SaveFileHandler *sfh, uint32_t error)
{
	ck_assert_uint_eq(error, E_NO_ERROR);
	*sfh->fh.memh.pos = 0;
	if(expected_output)
		ck_assert_str_eq(sfh->fh.memh.buffer, expected_output);
	return SaveBufferIO_close(sfh, E_NO_ERROR);
}


struct SaveFileHandler *SetupSaveTestIO(void)
{
	return SetupSaveBufferIO(SaveBufferIO_open, Test_BufferIO_Close);
}


bool verifying_error_handler(uint32_t type, struct SGFInfo *sgfi, va_list arglist)
{
	if(type == expected_error)
		expected_error_occurred = true;
	else if(type != E_NO_ERROR)
	{
		for(uint32_t *allowed=allowed_errors; allowed && *allowed; allowed++)
			if(type == *allowed)
				return true;
		ck_assert_msg(type == expected_error, "expected error: %lu (%x); received: %lu (%x)",
					  expected_error & M_ERROR_NUM, expected_error, type & M_ERROR_NUM, type);
	}
	return true;
}


void common_setup(void)
{
	sgfc = SetupSGFInfo(NULL, NULL);
	sgfc->options->add_sgfc_ap_property = false;
	/* run tests without PrintError (makes setup easier) */
	print_error_handler = NULL;
}

void common_teardown(void)
{
	/* buffer is assumed to be string literal, hence free() must not be called */
	sgfc->buffer = NULL;
	FreeSGFInfo(sgfc);
	print_error_handler = PrintErrorHandler;
}
