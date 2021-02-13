/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/test-helper.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

struct SGFInfo *sgfc;
char *expected_output;

int Test_BufferIO_Close(struct SaveFileHandler *sfh, U_LONG error)
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

void common_setup(void)
{
	sgfc = SetupSGFInfo(NULL);
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
