/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-move.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


START_TEST (test_ff4_pass_converted_on_19_board)
{
	struct TreeInfo info = {0};
	info.GM = 1;
	info.FF = 4;
	info.bwidth = 19;
	info.bheight = 19;
	sgfc->info = &info;

	char move[] = "tt";
	size_t len = 2;
	int ret = Parse_Move(move, &len, PARSE_MOVE, sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_int_eq(len, 0);
	ck_assert_int_eq(move[0], 0);
}
END_TEST


START_TEST (test_ff4_pass_kept_on_20_board)
{
	struct TreeInfo info = {0};
	info.GM = 1;
	info.FF = 4;
	info.bwidth = 20;
	info.bheight = 20;
	sgfc->info = &info;

	char move[] = "tt";
	size_t len = 2;
	int ret = Parse_Move(move, &len, PARSE_MOVE, sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_int_eq(len, 2);
	ck_assert_str_eq(move, "tt");
}
END_TEST


START_TEST (test_empty_pass_in_old_ff)
{
	struct TreeInfo info = {0};
	info.GM = 1;
	info.FF = 3;
	info.bwidth = 19;
	info.bheight = 19;
	sgfc->info = &info;

	char move[] = "   ";
	size_t len = 3;
	int ret = Parse_Move(move, &len, PARSE_MOVE, sgfc);
	ck_assert_int_eq(ret, -101);
	ck_assert_int_eq(len, 0);
}
END_TEST


START_TEST (test_non_go_move_keeps_text_but_drops_ctrl_bytes)
{
	struct TreeInfo info = {0};
	info.GM = 2;
	sgfc->info = &info;

	char move[] = {'a', 0, 'b', 0};
	size_t len = 3;
	int ret = Parse_Move(move, &len, PARSE_MOVE, sgfc);
	ck_assert_int_eq(ret, -1);
	ck_assert_int_eq(len, 2);
	ck_assert_str_eq(move, "ab");
}
END_TEST


TCase *sgfc_tc_parse_move(void)
{
	TCase *tc;

	tc = tcase_create("parse_move");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_ff4_pass_converted_on_19_board);
	tcase_add_test(tc, test_ff4_pass_kept_on_20_board);
	tcase_add_test(tc, test_empty_pass_in_old_ff);
	tcase_add_test(tc, test_non_go_move_keeps_text_but_drops_ctrl_bytes);
	return tc;
}
