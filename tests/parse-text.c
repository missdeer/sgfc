/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

START_TEST (test_basic_string)
{
    char text[] = "basic test";
    int len = Parse_Text(text, 0);
	ck_assert_int_eq(len, 10);
	ck_assert_str_eq(text, "basic test");
}
END_TEST

START_TEST (test_soft_linebreak)
{
    char text[] = "soft\\\nbreak";
    Parse_Text(text, 0);
	ck_assert_str_eq(text, "softbreak");

    char text2[] = "soft\\\nbreak:SIMPLE";
    Parse_Text(text2, PVT_SIMPLE);
	ck_assert_str_eq(text2, "softbreak:SIMPLE");
}
END_TEST

START_TEST (test_trailing_spaces)
{
    char text[] = "trailing   ";
    Parse_Text(text, 0);
	ck_assert_str_eq(text, "trailing");

    char text2[] = "trailing2 \\ \\ ";
    Parse_Text(text2, 0);
	ck_assert_str_eq(text2, "trailing2");

    char text3[] = "trailing3 \\\\ ";
    Parse_Text(text3, 0);
	ck_assert_str_eq(text3, "trailing3 \\\\");

    char text4[] = "trailing4 \\\\\\ ";
    Parse_Text(text4, 0);
	ck_assert_str_eq(text4, "trailing4 \\\\");
}
END_TEST

START_TEST (test_trailing_spaces_simpletext)
{
    char text[] = "trailing \n ";
    Parse_Text(text, PVT_SIMPLE);
	ck_assert_str_eq(text, "trailing");

    char text2[] = "trailing2 \n\n\\ \\\n ";
    Parse_Text(text2, PVT_SIMPLE);
	ck_assert_str_eq(text2, "trailing2");

    char text3[] = "trailing3 \r\n\\\r\\\n\n\r ";
    Parse_Text(text3, PVT_SIMPLE);
	ck_assert_str_eq(text3, "trailing3");
}
END_TEST


TCase *sgfc_tc_parse_prop_values ()
{
	TCase *tc;

	tc = tcase_create("prop_values");
	tcase_add_test(tc, test_basic_string);
	tcase_add_test(tc, test_soft_linebreak);
	tcase_add_test(tc, test_trailing_spaces);
	tcase_add_test(tc, test_trailing_spaces_simpletext);
	return tc;
}
