/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/options.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


START_TEST (test_one_filename)
	const char *args[] = {"sgfc", "input"};
	bool result = ParseArgs(sgfc, 2, args);
	ck_assert_msg(result == true);
	ck_assert_str_eq(sgfc->options->infile, "input");
END_TEST


START_TEST (test_two_filenames)
	const char *args[] = {"sgfc", "input", "output"};
	bool result = ParseArgs(sgfc, 3, args);
	ck_assert_msg(result == true);
	ck_assert_str_eq(sgfc->options->infile, "input");
	ck_assert_str_eq(sgfc->options->outfile, "output");
END_TEST


START_TEST (test_one_filename_with_leading_dash)
	const char *args[] = {"sgfc", "--", "-input"};
	bool result = ParseArgs(sgfc, 3, args);
	ck_assert_msg(result == true);
	ck_assert_str_eq(sgfc->options->infile, "-input");
END_TEST


START_TEST (test_two_filesnames_with_leading_dash)
	const char *argv2[] = {"sgfc", "--", "-input2", "-output"};
	bool result = ParseArgs(sgfc, 4, argv2);
	ck_assert_msg(result == true);
	ck_assert_str_eq(sgfc->options->infile, "-input2");
	ck_assert_str_eq(sgfc->options->outfile, "-output");
END_TEST


START_TEST (test_property_ids)
	const char *args[] = {"sgfc", "-yV", "-yTR"};
	bool result = ParseArgs(sgfc, 3, args);
	ck_assert_msg(result == true);
	ck_assert_msg(sgfc->options->delete_property[TKN_V] == true);
	ck_assert_msg(sgfc->options->delete_property[TKN_TR] == true);
END_TEST


START_TEST (test_int_options)
	const char *args[] = {"sgfc", "-E3l4", "-b2d7", "-d12d14d22", "-d34"};
	bool result = ParseArgs(sgfc, 5, args);
	ck_assert_msg(result == true);
	ck_assert_msg(sgfc->options->encoding == OPTION_ENCODING_NONE);
	ck_assert_msg(sgfc->options->linebreaks == OPTION_LINEBREAK_PRGRPH);
	ck_assert_msg(sgfc->options->find_start == OPTION_FINDSTART_SPEC);
	ck_assert_msg(sgfc->options->error_enabled[6] == false);
	ck_assert_msg(sgfc->options->error_enabled[11] == false);
	ck_assert_msg(sgfc->options->error_enabled[13] == false);
	ck_assert_msg(sgfc->options->error_enabled[21] == false);
	ck_assert_msg(sgfc->options->error_enabled[33] == false);
	/* just to make sure that at other options are true */
	ck_assert_msg(sgfc->options->error_enabled[3] == true);
END_TEST


START_TEST (test_bool_options)
	const char *args[] = {"sgfc", "-pet", "-c"};
	ck_assert_msg(sgfc->options->soft_linebreaks == true);
	bool result = ParseArgs(sgfc, 3, args);
	ck_assert_msg(result == true);
	ck_assert_msg(sgfc->options->pass_tt == true);
	ck_assert_msg(sgfc->options->expand_cpl == true);
	ck_assert_msg(sgfc->options->soft_linebreaks == false);
	ck_assert_msg(sgfc->options->write_critical == true);
END_TEST


START_TEST (test_long_options_and_encoding)
	const char *args[] = {"sgfc", "--version", "--encoding=UTF-8", "--default-encoding=ISO-8859-1"};
	bool result = ParseArgs(sgfc, 4, args);
	ck_assert_msg(result == true);
	ck_assert_msg(sgfc->options->help == OPTION_HELP_VERSION);
	ck_assert_str_eq(sgfc->options->default_encoding, "ISO-8859-1");
	ck_assert_msg(sgfc->options->forced_encoding, "UTF-8");
END_TEST


START_TEST (test_mix1)
	const char *args[] = {"sgfc", "input", "-mnd12yCHo", "--encoding=GB18030", "output"};
	bool result = ParseArgs(sgfc, 5, args);
	ck_assert_msg(result == true);
	ck_assert_msg(sgfc->options->del_move_markup == true);
	ck_assert_msg(sgfc->options->del_empty_nodes == true);
	ck_assert_msg(sgfc->options->keep_obsolete_props == false);
	ck_assert_msg(sgfc->options->error_enabled[11] == false);
	ck_assert_msg(sgfc->options->delete_property[TKN_CH] == true);
	ck_assert_str_eq(sgfc->options->forced_encoding, "GB18030");
	ck_assert_msg(sgfc->options->infile, "input");
	ck_assert_msg(sgfc->options->outfile, "output");
END_TEST


START_TEST (test_mix2)
	const char *args[] = {"sgfc", "-Uu", "input", "-vwl2", "--", "-output"};
	bool result = ParseArgs(sgfc, 6, args);
	ck_assert_msg(result == true);
	ck_assert_str_eq(sgfc->options->default_encoding, "UTF-8");
	ck_assert_msg(sgfc->options->keep_unknown_props == false);
	ck_assert_msg(sgfc->options->fix_variation == true);
	ck_assert_msg(sgfc->options->warnings == false);
	ck_assert_msg(sgfc->options->linebreaks == OPTION_LINEBREAK_NOSPACE);
	ck_assert_msg(sgfc->options->infile, "input");
	ck_assert_msg(sgfc->options->outfile, "-output");
END_TEST


TCase *sgfc_tc_options(void)
{
	TCase *tc;

	tc = tcase_create("options");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_one_filename);
	tcase_add_test(tc, test_two_filenames);
	tcase_add_test(tc, test_one_filename_with_leading_dash);
	tcase_add_test(tc, test_two_filesnames_with_leading_dash);
	tcase_add_test(tc, test_property_ids);
	tcase_add_test(tc, test_int_options);
	tcase_add_test(tc, test_bool_options);
	tcase_add_test(tc, test_long_options_and_encoding);
	tcase_add_test(tc, test_mix1);
	tcase_add_test(tc, test_mix2);
	return tc;
}
