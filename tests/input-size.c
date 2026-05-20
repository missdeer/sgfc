/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/input-size.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <check.h>

#include "test-common.h"


#define TEST_MAX_INPUT_SIZE 109


static void WriteBytes(FILE *file, size_t size)
{
	char buffer[TEST_MAX_INPUT_SIZE+10];
	char *head = "prelude (;FF[4]";
	char *tail = ";B[aa])";

	memset(buffer, ' ', TEST_MAX_INPUT_SIZE+10);
	memcpy(buffer, head, strlen(head));							/* NOLINT */
	memcpy(buffer + size - strlen(tail), tail, strlen(tail));	/* NOLINT */

	ck_assert_uint_eq(size, fwrite(buffer, 1, size, file));
	ck_assert_uint_eq(fflush(file), 0);
}


static void WriteTestFile(char *path, size_t path_size, int id, size_t size)
{
	/* The proper way would be using mkstemp(), but MSVC doesn't yet support it. */
	ck_assert_int_gt(snprintf(path, path_size, "/tmp/sgfc-input-size-%ld-%d",
							  (long)getpid(), id), 0);
	FILE *file = fopen(path, "wb");
	ck_assert_ptr_nonnull(file);
	WriteBytes(file, size);
	ck_assert_int_eq(fclose(file), 0);
}


START_TEST (test_source_file_too_large)
{
	char path[64];

	WriteTestFile(path, sizeof(path), 1, TEST_MAX_INPUT_SIZE+1);

	ck_assert_int_eq(LoadSGF(sgfc, path), false);
	ck_assert_int_eq(expected_error_occurred, true);

	ck_assert_int_eq(unlink(path), 0);
}
END_TEST


START_TEST (test_source_file_at_limit)
{
	char path[64];

	WriteTestFile(path, sizeof(path), 2, TEST_MAX_INPUT_SIZE);

	ck_assert_int_eq(LoadSGF(sgfc, path), true);
	ck_assert_int_eq(expected_error_occurred, false);

	ck_assert_int_eq(unlink(path), 0);
}
END_TEST


START_TEST (test_stdin_too_large)
{
	int stdin_copy = dup(STDIN_FILENO);
	ck_assert_int_ne(stdin_copy, -1);

	FILE *input = tmpfile();
	ck_assert_ptr_nonnull(input);
	WriteBytes(input, TEST_MAX_INPUT_SIZE+1);
	ck_assert_int_eq(fseek(input, 0L, SEEK_SET), 0);
	ck_assert_int_ne(dup2(fileno(input), STDIN_FILENO), -1);

	ck_assert_int_eq(LoadSGFFromStdin(sgfc), false);
	ck_assert_int_eq(expected_error_occurred, true);

	ck_assert_int_ne(dup2(stdin_copy, STDIN_FILENO), -1);
	close(stdin_copy);
	fclose(input);
}
END_TEST


START_TEST (test_stdin_at_limit)
{
	int stdin_copy = dup(STDIN_FILENO);
	ck_assert_int_ne(stdin_copy, -1);

	FILE *input = tmpfile();
	ck_assert_ptr_nonnull(input);
	WriteBytes(input, TEST_MAX_INPUT_SIZE);
	ck_assert_int_eq(fseek(input, 0L, SEEK_SET), 0);
	ck_assert_int_ne(dup2(fileno(input), STDIN_FILENO), -1);

	ck_assert_int_eq(LoadSGFFromStdin(sgfc), true);

	ck_assert_int_ne(dup2(stdin_copy, STDIN_FILENO), -1);
	close(stdin_copy);
	fclose(input);
}
END_TEST


static void input_size_setup(void)
{
	common_setup();

	expected_error = FE_SOURCE_TOO_LARGE;
	expected_error_occurred = false;
	print_error_handler = verifying_error_handler;
	sgfc->config->max_input_size = TEST_MAX_INPUT_SIZE;
}


static void input_size_teardown(void)
{
	free(sgfc->buffer);
	sgfc->buffer = NULL;
	common_teardown();
}


TCase *sgfc_tc_input_size(void)
{
	TCase *tc;

	tc = tcase_create("input_size");
	tcase_add_checked_fixture(tc, input_size_setup, input_size_teardown);

	tcase_add_test(tc, test_source_file_too_large);
	tcase_add_test(tc, test_source_file_at_limit);
	tcase_add_test(tc, test_stdin_too_large);
	tcase_add_test(tc, test_stdin_at_limit);
	return tc;
}
