/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/position.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

FILE *testout;

static char *ReadTestFile(const char *path, size_t *length)
{
	FILE *file = fopen(path, "rb");
	ck_assert_msg(!!file, "could not open file %s", path);
	/* being lazy: we know that all files are smaller than 10000 bytes */
	char *buffer = SaveMalloc(10000, "test file buffer");
	*length = fread(buffer, 1, 10000, file);
	fclose(file);
	return buffer;
}

static void FileTestOutput(struct SGFCError *error)
{
	CommonPrintErrorOutputHook(error, testout);
}

static void FileTestSetup(void)
{
	sgfc = SetupSGFInfo(NULL);
	testout = tmpfile();
	print_error_output_hook = FileTestOutput;
}

static void FileTestTeardown(void)
{
	FreeSGFInfo(sgfc);
	fclose(testout);
	print_error_output_hook = PrintErrorOutputHook;
}

static void TestWithFile(const char *path, const char *expected, char *output)
{
	int ret = LoadSGF(sgfc, path);
	ck_assert_int_eq(ret, true);
	ParseSGF(sgfc);

	size_t explen;
	expected_output = ReadTestFile(expected, &explen);
	ck_assert_uint_gt(explen, 150); /* smallest file ~ 190 byte */
	*(expected_output+explen) = 0;
	ret = SaveSGF(sgfc, SetupSaveTestIO, "outfile");
	ck_assert_int_eq(ret, true);
	free(expected_output);

	expected_output = ReadTestFile(output, &explen);
	ck_assert_uint_gt(explen, 190); /* smallest output ~ 190 byte */
	*(expected_output+explen) = 0;
	size_t actual_size = (size_t)ftell(testout);
	ck_assert_uint_gt(actual_size, explen - 70); /* longest summary line ~63 bytes */
	char *outbuf = SaveMalloc(actual_size, "stdout buffer");
	ck_assert_int_ne(-1, fseek(testout, 0, SEEK_SET));
	ck_assert_uint_eq(actual_size, fread(outbuf, 1, actual_size, testout));
	/* by only comparing up to actual_size we do not compare summary line */
	ck_assert_int_eq(0, memcmp(outbuf, expected_output, actual_size));
	free(outbuf);
	free(expected_output);
}


START_TEST (test_test_sgf)
	TestWithFile("../test-files/test.sgf",
			     "../test-files/test-result.sgf",
			     "../test-files/test-output.txt");
END_TEST


START_TEST (test_roun_test_sgf)
	sgfc->options->strict_checking = true;
	sgfc->options->keep_unknown_props = false;
	sgfc->options->keep_obsolete_props = false;
	sgfc->options->del_empty_nodes = true;
	TestWithFile("../test-files/test.sgf",
			     "../test-files/test-roun-result.sgf",
			     "../test-files/test-roun-output.txt");
END_TEST


START_TEST (test_strict_sgf)
	sgfc->options->strict_checking = true;
	TestWithFile("../test-files/strict.sgf",
			     "../test-files/strict-result.sgf",
			     "../test-files/strict-output.txt");
END_TEST


START_TEST (test_reorder_sgf)
	sgfc->options->fix_variation = true;
	TestWithFile("../test-files/reorder.sgf",
			     "../test-files/reorder-result.sgf",
			     "../test-files/reorder-output.txt");
END_TEST


START_TEST (test_reverse_reorder_sgf)
	sgfc->options->fix_variation = true;
	sgfc->options->reorder_variations = true;
	TestWithFile("../test-files/reorder.sgf",
			     "../test-files/reorder-z-result.sgf",
			     "../test-files/reorder-z-output.txt");
END_TEST


START_TEST (test_mixed_encoding_sgf)
	sgfc->options->encoding = OPTION_ENCODING_TEXT_ONLY;
	TestWithFile("../test-files/mixed-encoding.sgf",
			     "../test-files/mixed-encoding-result.sgf",
			     "../test-files/mixed-encoding-output.txt");
END_TEST


START_TEST (test_escaping_sgf)
	sgfc->options->soft_linebreaks = false;
	TestWithFile("../test-files/escaping.sgf",
			     "../test-files/escaping-result.sgf",
			     "../test-files/escaping-output.txt");
END_TEST


TCase *sgfc_tc_test_files(void)
{
	TCase *tc;

	tc = tcase_create("test-files");
	tcase_add_checked_fixture(tc, FileTestSetup, FileTestTeardown);

	tcase_add_test(tc, test_test_sgf);
	tcase_add_test(tc, test_roun_test_sgf);
	tcase_add_test(tc, test_strict_sgf);
	tcase_add_test(tc, test_reorder_sgf);
	tcase_add_test(tc, test_reverse_reorder_sgf);
	tcase_add_test(tc, test_mixed_encoding_sgf);
	tcase_add_test(tc, test_escaping_sgf);
	return tc;
}
