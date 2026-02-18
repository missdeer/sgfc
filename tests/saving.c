/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/saving.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


int Large_BufferIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	ck_assert_uint_eq(error, E_NO_ERROR);

	int i = 0;
	while (sfh->fh.memh.buffer[i] != 0 && expected_output[i] != 0 && sfh->fh.memh.buffer[i] == expected_output[i]) {
		i++;
	}
	int err_pos = i >= 20 ? i - 20 : 0;

	ck_assert_msg(sfh->fh.memh.buffer[i] == 0 && expected_output[i] == 0,
		"Result ('%.40s') differs from expected output ('%.40s') [position: %d]",
		&sfh->fh.memh.buffer[err_pos], &expected_output[err_pos], i);

	return SaveBufferIO_close(sfh, E_NO_ERROR);
}

struct SaveFileHandler *SetupLargeSaveTestIO(void)
{
	return SetupSaveBufferIO(SaveBufferIO_open, Large_BufferIO_Close);
}

START_TEST (test_extend_save_buffer)
{
	char buffer[DEFAULT_BUFFER_SIZE + 100];

	memset(buffer, 'x', DEFAULT_BUFFER_SIZE + 100);   // sentinel value
	strcpy(buffer, "(;FF[4]CA[UTF-8]GM[1]SZ[19]XX["); // fragile: header as output by SGFC
	buffer[strlen(buffer)] = 'x';
	strcpy(&buffer[DEFAULT_BUFFER_SIZE + 95], "]\n)\n");
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ret = ParseSGF(sgfc);
	ck_assert_int_eq(ret, true);

	expected_output = buffer;
	SaveSGF(sgfc, SetupLargeSaveTestIO, "outfile");
}
END_TEST


TCase *sgfc_tc_save(void)
{
	TCase *tc;

	tc = tcase_create("saving");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_extend_save_buffer);
	return tc;
}
