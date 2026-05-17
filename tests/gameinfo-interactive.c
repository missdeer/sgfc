/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/gameinfo-interactive.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <check.h>

#include "test-common.h"


static void RedirectPromptIO(const char *input, int *stdin_copy, int *stdout_copy,
							 FILE **tmp_in, FILE **tmp_out)
{
	*stdin_copy = dup(STDIN_FILENO);
	*stdout_copy = dup(STDOUT_FILENO);
	*tmp_in = tmpfile();
	*tmp_out = tmpfile();
	ck_assert_ptr_nonnull(*tmp_in);
	ck_assert_ptr_nonnull(*tmp_out);
	fwrite(input, 1, strlen(input), *tmp_in);
	ck_assert_int_eq(fseek(*tmp_in, 0L, SEEK_SET), 0);

	int fno_in = fileno(*tmp_in);
	int fno_out = fileno(*tmp_out);
	ck_assert_int_ne(fno_in, -1);
	ck_assert_int_ne(fno_out, -1);
	ck_assert_int_ne(dup2(fileno(*tmp_in), STDIN_FILENO), -1);
	ck_assert_int_ne(dup2(fileno(*tmp_out), STDOUT_FILENO), -1);
}


static void RestorePromptIO(int stdin_copy, int stdout_copy, FILE *tmp_in, FILE *tmp_out)
{
	fflush(stdout);
	ck_assert_int_gt(stdin_copy, 0);
	ck_assert_int_gt(stdout_copy, 0);
	dup2(stdin_copy, STDIN_FILENO);
	dup2(stdout_copy, STDOUT_FILENO);
	close(stdin_copy);
	close(stdout_copy);
	fclose(tmp_in);
	fclose(tmp_out);
}


START_TEST (test_interactive_invalid_result_keeps_value)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(result);
	ck_assert_str_eq(v.value, value);
	ck_assert_uint_eq(v.value_len, strlen(value));

	free(v.value);
}
END_TEST


START_TEST (test_interactive_corrected_result_keeps_value)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "White wins by 3 1/2 points";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(result);
	ck_assert_str_eq(v.value, "W+3.5");
	ck_assert_uint_eq(v.value_len, 5);

	free(v.value);
}
END_TEST


START_TEST (test_interactive_corrected_input)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "White wins by 3 points";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("Black wins by 4\n\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(result);
	ck_assert_str_eq(v.value, "B+4");
	ck_assert_uint_eq(v.value_len, 3);

	free(v.value);
}
END_TEST


START_TEST (test_interactive_delete_value)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "White wins by 3 points";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("d\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(!result);

	free(v.value);
}
END_TEST


START_TEST (test_interactive_long_time_input_has_correction_slack)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "bad time";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_TM, .idstr = "TM", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("12345678901234567890123456h\n\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(result);
	ck_assert_uint_eq(v.value_len, strlen(v.value));
	ck_assert_uint_gt(v.value_len, strlen("12345678901234567890123456h"));
	ck_assert_uint_eq(strspn(v.value, "0123456789."), v.value_len);

	free(v.value);
}
END_TEST


START_TEST (test_interactive_long_result_input_has_correction_slack)
{
	int stdin_copy, stdout_copy;
	FILE *tmp_in, *tmp_out;
	char value[] = "bad result";
	struct PropValue v = { .value = SafeDupString(value, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("B+12345678901234567890123456half\n\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(result);
	ck_assert_uint_eq(v.value_len, strlen(v.value));
	ck_assert_uint_eq(v.value[0], 'B');
	ck_assert_uint_eq(v.value[1], '+');
	ck_assert_uint_eq(strspn(&v.value[2], "0123456789."), v.value_len - 2);

	free(v.value);
}
END_TEST


TCase *sgfc_tc_gameinfo_interactive(void)
{
	TCase *tc;

	tc = tcase_create("gameinfo_interactive");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_interactive_invalid_result_keeps_value);
	tcase_add_test(tc, test_interactive_corrected_result_keeps_value);
	tcase_add_test(tc, test_interactive_corrected_input);
	tcase_add_test(tc, test_interactive_delete_value);
	tcase_add_test(tc, test_interactive_long_time_input_has_correction_slack);
	tcase_add_test(tc, test_interactive_long_result_input_has_correction_slack);
	return tc;
}
