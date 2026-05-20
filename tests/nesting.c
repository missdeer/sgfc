/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/nesting.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>

#include "test-common.h"


START_TEST (test_deep_nesting_limit)
{
	char buffer[DEFAULT_TREE_NESTING_LIMIT*2+10] = {0};

	for(ptrdiff_t i=0; i <= DEFAULT_TREE_NESTING_LIMIT; i++)
	{
		buffer[i*2] = '(';
		buffer[i*2+1] = ';';
	}
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
}
END_TEST


START_TEST (test_deeper_nesting_limit)
{
	char buffer[DEFAULT_TREE_NESTING_LIMIT*2+10] = {0};

	for(ptrdiff_t i=0; i <= DEFAULT_TREE_NESTING_LIMIT+1; i++)
	{
		buffer[i*2] = '(';
		buffer[i*2+1] = ';';
	}
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, false);
}
END_TEST


TCase *sgfc_tc_tree_nesting(void)
{
	TCase *tc;

	tc = tcase_create("tree_nesting");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_deep_nesting_limit);
	tcase_add_test(tc, test_deeper_nesting_limit);
	return tc;
}
