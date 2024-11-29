/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/encoding.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <iconv.h>

#include "test-common.h"


START_TEST (test_detect_encoding_BOM)
{
	char *result;
	char buffer[4] = {'\xFE', '\xFF', ' ', ' '};

	result = DetectEncoding(buffer, buffer+4);
	ck_assert_str_eq(result, "UTF-16BE");
	free(result);

	buffer[0] = '\xFF';
	buffer[1] = '\xFE';
	result = DetectEncoding(buffer, buffer+4);
	ck_assert_str_eq(result, "UTF-16LE");
	free(result);

	buffer[2] = 0;
	buffer[3] = 0;
	result = DetectEncoding(buffer, buffer+4);
	ck_assert_str_eq(result, "UTF-32LE");
	free(result);

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = '\xFE';
	buffer[3] = '\xFF';
	result = DetectEncoding(buffer, buffer+4);
	ck_assert_str_eq(result, "UTF-32BE");
	free(result);

	buffer[0] = '\xEF';
	buffer[1] = '\xBB';
	buffer[2] = '\xBF';
	buffer[3] = '\n';
	result = DetectEncoding(buffer, buffer+4);
	ck_assert_str_eq(result, "UTF-8");
	free(result);
}
END_TEST


START_TEST (test_detect_encoding)
{
	char *result;

	char buffer[] = "some (text CA[basic-case] more text";
	result = DetectEncoding(buffer, buffer + strlen(buffer));
	ck_assert_str_eq(result, "basic-case");
	free(result);

	char buffer2[] = "some (CA\n [ spaces \n] ";
	result = DetectEncoding(buffer2, buffer2 + strlen(buffer2));
	ck_assert_str_eq(result, "spaces");
	free(result);

	char buffer3[] = "some text in (front ClowerAcase\n [ lower-case]";
	result = DetectEncoding(buffer3, buffer3 + strlen(buffer3));
	ck_assert_str_eq(result, "lower-case");
	free(result);

	char buffer4[] = "(CCA[one]CA[second]";
	result = DetectEncoding(buffer4, buffer4 + strlen(buffer4));
	ck_assert_str_eq(result, "second");
	free(result);

	char buffer5[] = "(xCyAzA[one]CxA[second-lower]";
	result = DetectEncoding(buffer5, buffer5 + strlen(buffer5));
	ck_assert_str_eq(result, "second-lower");
	free(result);

	char buffer6[] = "(xCyA.CzA[word-boundary] more";
	result = DetectEncoding(buffer6, buffer6 + strlen(buffer6));
	ck_assert_str_eq(result, "word-boundary");
	free(result);

	char buffer7[] = "no:CA[one] (CA[after-brace]";
	result = DetectEncoding(buffer7, buffer7 + strlen(buffer7));
	ck_assert_str_eq(result, "after-brace");
	free(result);
}
END_TEST


START_TEST (test_no_encoding_specified)
{
	char *result;

	char buffer[] = "you're not gonna find it";
	result = DetectEncoding(buffer, buffer + strlen(buffer));
	ck_assert_ptr_eq(result, NULL);

	char buffer2[] = "you're not gonna CA[it";
	result = DetectEncoding(buffer2, buffer2 + strlen(buffer2));
	ck_assert_ptr_eq(result, NULL);
}
END_TEST


START_TEST (test_basic_conversion)
{
	char src_buffer[] = "simple test";
	char dst_buffer[100];
	char *src_pos, *dst_pos, *result;
	size_t src_left, dst_left = 100;
	iconv_t cd;

	/* convert buffer to UTF-16LE encoding */
	src_left = strlen(src_buffer);
	src_pos = src_buffer;
	dst_pos = dst_buffer;
	cd = iconv_open("UTF-16LE", "UTF-8");
	iconv(cd, &src_pos, &src_left, &dst_pos, &dst_left);
	iconv_close(cd);

	/* ... and convert text back */
	cd = iconv_open("UTF-8", "UTF-16");
	result = DecodeBuffer(sgfc, cd, dst_buffer, (U_LONG)(dst_pos - dst_buffer), 0, NULL);
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, src_buffer);
	free(result);
	iconv_close(cd);
}
END_TEST


START_TEST (test_bad_char_conversion)
{
	char *result;
	char buffer[50]; /* large enough so that no reallocation is necessary */
	memset(buffer, 0xFF, 50);
	/* 01234567890123456789012345; fill rest with \xFF; no \0! */
	strncpy(buffer, "simple test with bad chars", 26);
	buffer[3] = '\xF0';		/* single bad byte */
	buffer[8] = '\xC0';		/* 2 byte seq without second byte */
	buffer[13] = '\xE2';	/* 3 byte seq without third byte */
	buffer[14] = '\x82';
	buffer[18] = '\x81';	/* single continuation byte */

	/* UTF-8 -> UTF-8 eliminates bad chars */
	iconv_t cd = iconv_open("UTF-8", "UTF-8");
	result = DecodeBuffer(sgfc, cd, buffer, 50, 0, NULL);

	ck_assert_ptr_ne(result, NULL);
	/* Test assumes UTF-8 encoded string literals */
	ck_assert_str_eq(result, "sim\uFFFDle t\uFFFDst w\uFFFDh b\uFFFDd chars\uFFFD");
	free(result);
	iconv_close(cd);
}
END_TEST


START_TEST (test_buffer_overflow_conversion)
{
	char *result;

	char buffer[2] = {'\xE4', 0}; /* "ä" in ISO-8859-1 */
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + 1;
	iconv_t cd = iconv_open("UTF-8", "ISO-8859-1");
	result = DecodeBuffer(sgfc, cd, buffer, 1, 0, NULL);
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "\u00E4");
	free(result);

	char buffer2[1000];				/* bad case for calculation of "needed" bytes: */
	memset(buffer2, ' ', 950);		/* no expansion first */
	memset(buffer2+950, 0xE4, 50);	/* then double expansions */
	result = DecodeBuffer(sgfc, cd, buffer2, 1000, 0, NULL);
	ck_assert_ptr_ne(result, NULL);
	char expected[1051];
	memset(expected, ' ', 950);
	for(int i=0; i < 50; i++) {
		expected[950+i*2]	= '\xC3';	/* 2 byte UTF-8 encoding of 'ä' */
		expected[950+i*2+1] = '\xA4';
	}
	expected[1050] = 0;
	ck_assert_str_eq(result, expected);
	iconv_close(cd);
	free(result);
}
END_TEST


START_TEST (test_8bit_value_in_middle)
{
	print_error_handler = PrintErrorHandler;	/* count errors */
	print_error_output_hook = NULL;
	sgfc->options->forced_encoding = "UTF-8";
	sgfc->options->encoding = OPTION_ENCODING_EVERYTHING;
	/* UTF-8 of U+4E2D (中) */
	char buffer[] = "(;AP[abc\xE4\xB8\xADxyz:def\xE4\xB8\xADuvw]C[ab\xE4\xB8\xADxyz])";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("abc\xE4\xB8\xADxyz", sgfc->root->prop->value->value);
	ck_assert_str_eq("def\xE4\xB8\xADuvw", sgfc->root->prop->value->value2);
	ck_assert_str_eq("ab\xE4\xB8\xADxyz", sgfc->root->prop->next->value->value);
	ck_assert_int_eq(3, (long)sgfc->root->prop->col);
	ck_assert_int_eq(22, (long)sgfc->root->prop->next->col);
	ck_assert_int_eq(0, sgfc->error_count);
	ck_assert_int_eq(0, sgfc->warning_count);
}
END_TEST


START_TEST (test_8bit_value_at_end)
{
	print_error_handler = PrintErrorHandler;	/* count errors */
	print_error_output_hook = NULL;
	sgfc->options->forced_encoding = "UTF-8";
	sgfc->options->encoding = OPTION_ENCODING_EVERYTHING;

	char buffer[] = "(;AP[\xE4\xB8\xAD:\xE5\xB8\xAE]C[ab\xE4\xB8\xAD])";	/* only 8-Bit & at end */
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ck_assert_str_eq("\xE4\xB8\xAD", sgfc->root->prop->value->value);
	ck_assert_str_eq("\xE5\xB8\xAE", sgfc->root->prop->value->value2);
	ck_assert_str_eq("ab\xE4\xB8\xAD", sgfc->root->prop->next->value->value);
	ck_assert_int_eq(3, (long)sgfc->root->prop->col);
	ck_assert_int_eq(10, (long)sgfc->root->prop->next->col);
	ck_assert_int_eq(0, sgfc->error_count);
	ck_assert_int_eq(0, sgfc->warning_count);
}
END_TEST


TCase *sgfc_tc_encoding(void)
{
	TCase *tc;

	tc = tcase_create("encoding");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_detect_encoding_BOM);
	tcase_add_test(tc, test_detect_encoding);
	tcase_add_test(tc, test_no_encoding_specified);
	tcase_add_test(tc, test_basic_conversion);
	tcase_add_test(tc, test_bad_char_conversion);
	tcase_add_test(tc, test_buffer_overflow_conversion);
	tcase_add_test(tc, test_8bit_value_in_middle);
	tcase_add_test(tc, test_8bit_value_at_end);
	return tc;
}
