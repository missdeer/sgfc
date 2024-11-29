/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/position.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


START_TEST (test_add_has_no_effect)
{
	char buffer[] = "(;B[aa];W[ab];AB[aa]AW[ab]AE[ac])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ParseSGF(sgfc);

	expected_output = "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[aa];W[ab];)\n";
	ret = SaveSGF(sgfc, SetupSaveTestIO, "outfile");
	ck_assert_int_eq(ret, true);
}
END_TEST


START_TEST (test_add_effect_across_variations)
{
	char buffer[] = "(; (;W[aa])  (;W[aa] (;B[bb])(;AE[aa])))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ParseSGF(sgfc);

	expected_output = "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n(;W[aa])\n(;W[aa]\n(;B[bb])\n(;AE[aa])))\n";
	ret = SaveSGF(sgfc, SetupSaveTestIO, "outfile");
	ck_assert_int_eq(ret, true);
}
END_TEST


TCase *sgfc_tc_position(void)
{
	TCase *tc;

	tc = tcase_create("position");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_add_has_no_effect);
	tcase_add_test(tc, test_add_effect_across_variations);
	return tc;
}
