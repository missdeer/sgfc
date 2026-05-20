/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/test-runner.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <check.h>

TCase *sgfc_tc_check_value(void);
TCase *sgfc_tc_delete_node(void);
TCase *sgfc_tc_encoding(void);
TCase *sgfc_tc_encoding2(void);
TCase *sgfc_tc_load_properties(void);
TCase *sgfc_tc_options(void);
TCase *sgfc_tc_other_games(void);
TCase *sgfc_tc_parse_move(void);
TCase *sgfc_tc_parse_text(void);
TCase *sgfc_tc_position(void);
TCase *sgfc_tc_test_files(void);
TCase *sgfc_tc_trigger_errors(void);
TCase *sgfc_tc_value_length(void);
TCase *sgfc_tc_save(void);
TCase *sgfc_tc_gameinfo_interactive(void);
TCase *sgfc_tc_gameinfo_correction(void);
TCase *sgfc_tc_tree_nesting(void);
TCase *sgfc_tc_input_size(void);


Suite *sgfc_suite(void)
{
	Suite *s = suite_create("SGFC");
	suite_add_tcase(s, sgfc_tc_check_value());
	suite_add_tcase(s, sgfc_tc_delete_node());
	suite_add_tcase(s, sgfc_tc_encoding());
	suite_add_tcase(s, sgfc_tc_encoding2());
	suite_add_tcase(s, sgfc_tc_load_properties());
	suite_add_tcase(s, sgfc_tc_options());
	suite_add_tcase(s, sgfc_tc_other_games());
	suite_add_tcase(s, sgfc_tc_parse_move());
	suite_add_tcase(s, sgfc_tc_parse_text());
	suite_add_tcase(s, sgfc_tc_position());
	suite_add_tcase(s, sgfc_tc_test_files());
	suite_add_tcase(s, sgfc_tc_trigger_errors());
	suite_add_tcase(s, sgfc_tc_value_length());
	suite_add_tcase(s, sgfc_tc_save());
	suite_add_tcase(s, sgfc_tc_gameinfo_interactive());
	suite_add_tcase(s, sgfc_tc_gameinfo_correction());
	suite_add_tcase(s, sgfc_tc_tree_nesting());
	suite_add_tcase(s, sgfc_tc_input_size());
	return s;
}


int main(void)
{
	int number_failed;

	Suite *s = sgfc_suite();
	SRunner *sr = srunner_create(s);
	/* switch to NO_FORK for easier debugging */
	/* srunner_set_fork_status(sr, CK_NOFORK); */
	srunner_run_all(sr, CK_ENV);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
