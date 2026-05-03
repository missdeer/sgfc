/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/gameinfo-interactive.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test-common.h"


static void RedirectPromptIO(const char *input, int *stdin_copy, int *stdout_copy,
							 FILE **tmp_in, FILE **tmp_out)
{
	*stdin_copy = dup(STDIN_FILENO);
	*stdout_copy = dup(STDOUT_FILENO);
	*tmp_in = tmpfile();
	*tmp_out = tmpfile();
	fwrite(input, 1, strlen(input), *tmp_in);
	rewind(*tmp_in);
	dup2(fileno(*tmp_in), STDIN_FILENO);
	dup2(fileno(*tmp_out), STDOUT_FILENO);
}


static void RestorePromptIO(int stdin_copy, int stdout_copy, FILE *tmp_in, FILE *tmp_out)
{
	fflush(stdout);
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
	struct PropValue v = { .value = SafeDupString(value, 0, "value"), .value_len = strlen(value) };
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
	struct PropValue v = { .value = SafeDupString(value, 0, "value"), .value_len = strlen(value) };
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
	struct PropValue v = { .value = SafeDupString(value, 0, "value"), .value_len = strlen(value) };
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
	struct PropValue v = { .value = SafeDupString(value, 0, "value"), .value_len = strlen(value) };
	struct Property p = { .id = TKN_RE, .idstr = "RE", .value = &v };

	sgfc->options->interactive = true;

	RedirectPromptIO("d\n", &stdin_copy, &stdout_copy, &tmp_in, &tmp_out);
	bool result = Check_GameInfo(sgfc, &p, &v);
	RestorePromptIO(stdin_copy, stdout_copy, tmp_in, tmp_out);

	ck_assert(!result);

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
	return tc;
}
