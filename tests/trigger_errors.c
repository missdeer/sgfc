/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

START_TEST (test_W_SGF_IN_HEADER)
{
	char sgf[] = "B[aa](;)";
	struct SGFInfo sgf =
	int len = Parse_Text(text, 0);
	ck_assert_int_eq(len, 10);
	ck_assert_str_eq(text, "basic test");
}
END_TEST


TCase *sgfc_tc_parse_text()
{
	TCase *tc;

	tc = tcase_create("parse_text");
	tcase_add_test(tc, test_basic_string);
	return tc;
}
