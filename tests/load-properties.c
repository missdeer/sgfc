/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/load-properties.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <string.h>

#include "test-common.h"


START_TEST (test_lowercase_in_front)
	char buffer[] = "(;ccB[aa])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("ccB", sgfc->root->prop->idstr);
END_TEST


START_TEST (test_lowercase_around)
	char buffer[] = "(;ccAddBee[aa])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("ccAddBee", sgfc->root->prop->idstr);
END_TEST


START_TEST (test_lowercase_second_prop)
	char buffer[] = "(;AB[aa]xxAEyy[bb])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("AB", sgfc->root->prop->idstr);
	ck_assert_str_eq("xxAEyy", sgfc->root->prop->next->idstr);
END_TEST


START_TEST (test_lowercase_missing_semicolon)
	char buffer[] = "(;AB[aa](xxAEyy[bb]))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("xxAEyy", sgfc->root->child->prop->idstr);
END_TEST


int test_lwic_errors_seen = -1;
struct SGFCError test_lwic_errors[] =
{
	/* 13x {err, msg, row, col, errno} */
	{E_ILLEGAL_OUTSIDE_CHARS, "\"xx\"", 1,  3, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"z3\"", 1, 15, 0},
	{E_NO_PROP_VALUES,		  "<zzZZ>", 1, 18, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"ww\"", 1, 30, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"cc\"", 1, 37, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"q_\"", 1, 44, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"kk\"", 2,  2, 0},
	{E_MISSING_NODE_START,	  "",		2,  5, 0},
	{E_ILLEGAL_OUTSIDE_CHARS, "\"ll\"", 3,  2, 0},
	{E_EMPTY_VARIATION,		  "",		3,  4, 0},
	{E_MISSING_NODE_START,	  "",		3,  7, 0},
	{E_NO_PROP_VALUES,		  "<rrR>",	3,  7, 0},
	{E_NO_PROP_VALUES,		  "<ggG>",	3, 14, 0},
};

void test_lwic_error_output(struct SGFCError *error)
{
	test_lwic_errors_seen++;
	ck_assert_msg(test_lwic_errors_seen <= 12, "too many errors, latest %lx at %d:%d:%s",
			   	  error->error, error->row, error->col, error->message);
	struct SGFCError expect = test_lwic_errors[test_lwic_errors_seen];
	ck_assert_uint_eq(error->error, expect.error);
	ck_assert_msg(strstr(error->message, expect.message) != NULL,
			      "should contain '%s': %s", expect.message, error->message);
	ck_assert_uint_eq(error->row, expect.row);
	ck_assert_uint_eq(error->col, expect.col);
}

START_TEST (test_lowercase_with_illegal_chars)
	char buffer[] = "(;xx yyAB[aa] z3 zzZZ uuAWvv ww[bb] cc[pp] q_ \n"
				    "(kk xxAEyy[bb])\n"
					"(ll) (rrR) (;ggG)\n"
	 				"(;ssBs[ab]))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	print_error_handler = PrintErrorHandler;
	print_error_output_hook = test_lwic_error_output;
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("yyAB", sgfc->root->prop->idstr);
	ck_assert_str_eq("uuAWvv", sgfc->root->prop->next->idstr);
	ck_assert_str_eq("pp", sgfc->root->prop->next->value->next->value);
	ck_assert_str_eq("xxAEyy", sgfc->root->child->prop->idstr);
	ck_assert_str_eq("ssBs", sgfc->root->child->sibling->sibling->sibling->prop->idstr);
	ck_assert_msg(test_lwic_errors_seen == 12,
			      "not all errors seen, expected 12, got %d", test_lwic_errors_seen);
END_TEST



TCase *sgfc_tc_load_properties(void)
{
	TCase *tc;

	tc = tcase_create("position");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_lowercase_in_front);
	tcase_add_test(tc, test_lowercase_around);
	tcase_add_test(tc, test_lowercase_second_prop);
	tcase_add_test(tc, test_lowercase_missing_semicolon);
	tcase_add_test(tc, test_lowercase_with_illegal_chars);
	return tc;
}
