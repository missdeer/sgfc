/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/delete-node.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <string.h>

#include "test-common.h"


START_TEST (test_delete_leaf_node)
	char buffer[] = "(;N[a];)";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);

	sgfc->options->del_empty_nodes = true;
	ParseSGF(sgfc);
	ck_assert_ptr_eq(NULL, sgfc->root->child);
END_TEST


START_TEST (test_delete_middle_node)
	char buffer[] = "(;N[a];;N[b])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);

	sgfc->options->del_empty_nodes = true;
	ParseSGF(sgfc);
	ck_assert_ptr_eq(NULL, sgfc->root->child->child);
	ck_assert_str_eq("b", sgfc->root->child->prop->value->value);
END_TEST


START_TEST (test_delete_root_node)
	char buffer[] = "(;;N[b])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);

	sgfc->options->del_empty_nodes = true;
	ParseSGF(sgfc);
	ck_assert_ptr_eq(NULL, sgfc->root->child);
	ck_assert_str_eq("b", sgfc->root->prop->value->value);
END_TEST


START_TEST (test_delete_with_sibling)
	char buffer[] = "(;N[a](;N[b];N[c])(;N[d]))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	InitAllTreeInfo(sgfc);	/* necessary for SaveSGF */

	DelNode(sgfc, sgfc->root->child, E_NO_ERROR);

	expected_output = "(;FF[4]CA[UTF-8]GM[1]SZ[19]N[a]\n(;N[c])\n(;N[d]))\n";
	SaveSGF(sgfc, "outfile");
END_TEST


START_TEST (test_delete_replace_with_sibling)
	char buffer[] = "(;N[a](;N[b])(;N[d]))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	InitAllTreeInfo(sgfc);	/* necessary for SaveSGF */

	DelNode(sgfc, sgfc->root->child, E_NO_ERROR);

	expected_output = "(;FF[4]CA[UTF-8]GM[1]SZ[19]N[a];N[d])\n";
	SaveSGF(sgfc, "outfile");
END_TEST


START_TEST (test_delete_fails)
	char buffer[] = "(;N[a](;N[b](;N[c1])(;N[c2]))(;N[d]))";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	InitAllTreeInfo(sgfc);	/* necessary for SaveSGF */

	DelNode(sgfc, sgfc->root->child, E_NO_ERROR);

	expected_output = "(;FF[4]CA[UTF-8]GM[1]SZ[19]N[a]\n(;N[b]\n(;N[c1])\n(;N[c2]))\n(;N[d]))\n";
	SaveSGF(sgfc, "outfile");
END_TEST


TCase *sgfc_tc_delete_node(void)
{
	TCase *tc;

	tc = tcase_create("position");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_delete_leaf_node);
	tcase_add_test(tc, test_delete_middle_node);
	tcase_add_test(tc, test_delete_root_node);
	tcase_add_test(tc, test_delete_with_sibling);
	tcase_add_test(tc, test_delete_replace_with_sibling);
	tcase_add_test(tc, test_delete_fails);
	return tc;
}
