/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/testrunner.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <check.h>

extern TCase *sgfc_tc_parse_text();

Suite *sgfc_suite(void)
{
	Suite *s = suite_create("SGFC");
	suite_add_tcase(s, sgfc_tc_parse_text());
	return s;
}


int main(void)
{
	int number_failed;

	Suite *s = sgfc_suite();
	SRunner *sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_ENV);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
