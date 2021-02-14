/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


START_TEST (test_move_property)
	char buffer[] = "(;GM[123];B[f45];W[g67:snake])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ret = ParseSGF(sgfc);
	ck_assert_int_eq(ret, true);

	/* check that values are not decomposed */
	ck_assert_str_eq("f45", sgfc->root->child->prop->value->value);
	ck_assert_str_eq("g67:snake", sgfc->root->child->child->prop->value->value);
	ck_assert_ptr_eq(NULL, sgfc->root->child->child->prop->value->value2);
END_TEST


START_TEST (test_stone_property)
	char buffer[] = "(;GM[123];AB[f45]AW[g67:rabbit])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ret = ParseSGF(sgfc);
	ck_assert_int_eq(ret, true);

	/* check that values are not decomposed */
	ck_assert_str_eq("f45", sgfc->root->child->prop->value->value);
	ck_assert_str_eq("g67:rabbit", sgfc->root->child->prop->next->value->value);
	ck_assert_ptr_eq(NULL, sgfc->root->child->prop->next->value->value2);
END_TEST


START_TEST (test_point_property)
	char buffer[] = "(;GM[123];MA[f45:h51]LB[g67:text];AE[i8:j~8])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ret = ParseSGF(sgfc);
	ck_assert_int_eq(ret, true);

	/* check for decomposed values */
	ck_assert_str_eq("f45", sgfc->root->child->prop->value->value);
	ck_assert_str_eq("h51", sgfc->root->child->prop->value->value2);
	ck_assert_str_eq("g67", sgfc->root->child->prop->next->value->value);
	ck_assert_str_eq("text", sgfc->root->child->prop->next->value->value2);
	ck_assert_str_eq("i8", sgfc->root->child->child->prop->value->value);
	ck_assert_str_eq("j~8", sgfc->root->child->child->prop->value->value2);
END_TEST


START_TEST (test_no_compressed_lists)
	/* just verify that SGFC doesn't try to expand compressed point lists */
	char buffer[] = "(;GM[123];AB[f11:turtle]AE[g33:h44])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	sgfc->options->expand_cpl = true;

	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ret = ParseSGF(sgfc);
	ck_assert_int_eq(ret, true);

	expected_output = "(;FF[4]CA[UTF-8]GM[123];AB[f11:turtle]AE[g33:h44])\n";
	SaveSGF(sgfc, SetupSaveTestIO, "outfile");
END_TEST


TCase *sgfc_tc_other_games(void)
{
	TCase *tc;

	tc = tcase_create("other_games");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_move_property);
	tcase_add_test(tc, test_stone_property);
	tcase_add_test(tc, test_point_property);
	tcase_add_test(tc, test_no_compressed_lists);
	return tc;
}
