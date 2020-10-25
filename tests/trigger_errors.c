/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

static struct SGFInfo *sgfc;
static U_LONG expected_error;
static bool expected_error_occurred;
static char *expected_output;

static int Test_BufferIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	ck_assert_uint_eq(error, E_NO_ERROR);
	*sfh->fh.memh.pos = 0;
	ck_assert_str_eq(sfh->fh.memh.buffer, expected_output);
	return(SaveFile_BufferIO_Close(sfh, E_NO_ERROR));
}

static int mock_error_handler(U_LONG type, struct SGFInfo *sgfi, va_list arglist)
{
	if(type != E_NO_ERROR)
	{
		ck_assert_msg(type == expected_error, "expected error: %d (%lx); received: %d (%lx)",
					  expected_error & M_ERROR_NUM, expected_error, type & M_ERROR_NUM, type);
		expected_error_occurred = TRUE;
	}
	return(TRUE);
}

static void setup(void)
{
	struct SaveFileHandler *sfh = Setup_SaveBufferIO(Test_BufferIO_Close);
	sgfc = Setup_SGFInfo(NULL, sfh);
	sgfc->options->add_sgfc_ap_property = FALSE;
	print_error_handler = mock_error_handler;
	expected_error_occurred = FALSE;
}

static void teardown(void)
{
	sgfc->buffer = NULL;
	FreeSGFInfo(sgfc);
}


static void trigger_error(U_LONG type, char *buffer, char *expected)
{
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	expected_output = expected;
	expected_error = type;
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, TRUE);
	ParseSGF(sgfc);
	ret = SaveSGF(sgfc, "outfile");
	ck_assert_int_eq(ret, TRUE);
	ck_assert_msg(expected_error_occurred);
}


START_TEST (test_W_SGF_IN_HEADER)
{
	trigger_error(W_SGF_IN_HEADER,
				  "B[aa](;)",
				  "(;FF[4]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_ILLEGAL_OUTSIDE_CHARS)
{
	trigger_error(E_ILLEGAL_OUTSIDE_CHARS,
				  "(; illegal )",
				  "(;FF[4]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_VARIATION_NESTING)
{
	trigger_error(E_VARIATION_NESTING,
				  "(;B[aa]",
				  "(;FF[4]GM[1]SZ[19]B[aa])\n");
}
END_TEST


START_TEST (test_E_UNEXPECTED_EOF)
{
	trigger_error(E_UNEXPECTED_EOF,
				  "(;B[aa",
				  "(;FF[4]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_PROPID_TOO_LONG)
{
	trigger_error(E_PROPID_TOO_LONG,
				  "(;XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX[])",
				  "(;FF[4]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_EMPTY_VARIATION)
{
	trigger_error(E_EMPTY_VARIATION,
				  "(;;B[aa]()(;W[bb]))",
				  "(;FF[4]GM[1]SZ[19];B[aa];W[bb])\n");
}
END_TEST


START_TEST (test_E_TOO_MANY_VALUES)
{
	trigger_error(E_TOO_MANY_VALUES,
				  "(;;B[aa][bb])",
				  "(;FF[4]GM[1]SZ[19];B[aa])\n");
}
END_TEST


START_TEST (test_WS_MOVE_IN_ROOT)
{
	sgfc->options->fix_variation = TRUE;
	trigger_error(WS_MOVE_IN_ROOT,
				  "(;B[aa])",
				  "(;FF[4]GM[1]SZ[19];B[aa])\n");
}
END_TEST


TCase *sgfc_tc_trigger_errors()
{
	TCase *tc;

	tc = tcase_create("trigger_errors");
	tcase_add_checked_fixture(tc, setup, teardown);

	tcase_add_test(tc, test_W_SGF_IN_HEADER);
	tcase_add_test(tc, test_E_ILLEGAL_OUTSIDE_CHARS);
	tcase_add_test(tc, test_E_VARIATION_NESTING);
	tcase_add_test(tc, test_E_UNEXPECTED_EOF);
	tcase_add_test(tc, test_E_PROPID_TOO_LONG);
	tcase_add_test(tc, test_E_EMPTY_VARIATION);
	tcase_add_test(tc, test_E_TOO_MANY_VALUES);
	tcase_add_test(tc, test_WS_MOVE_IN_ROOT);
	return tc;
}
