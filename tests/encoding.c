/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/encoding.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <iconv.h>

#include "test-common.h"


START_TEST (test_detect_encoding_BOM)
	char *result;
	char buffer[4] = {'\xFE', '\xFF', ' ', ' '};
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + 4;

	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "UTF-16BE");
	free(result);

	buffer[0] = '\xFF';
	buffer[1] = '\xFE';
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "UTF-16LE");
	free(result);

	buffer[2] = 0;
	buffer[3] = 0;
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "UTF-32LE");
	free(result);

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = '\xFE';
	buffer[3] = '\xFF';
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "UTF-32BE");
	free(result);

	buffer[0] = '\xEF';
	buffer[1] = '\xBB';
	buffer[2] = '\xBF';
	buffer[3] = '\n';
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "UTF-8");
	free(result);
END_TEST


START_TEST (test_detect_encoding)
	char *result;
	char buffer[] = "some (text CA[basic-case] more text";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "basic-case");
	free(result);

	strcpy(buffer, "some (CA\n [ spaces \n] ");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "spaces");
	free(result);

	char buffer2[] = "some text in (front ClowerAcase\n [ lower-case]";
	sgfc->buffer = buffer2;
	sgfc->b_end = buffer2 + strlen(buffer2);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "lower-case");
	free(result);

	strcpy(buffer2, "(CCA[one]CA[second]");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "second");
	free(result);

	strcpy(buffer2, "(xCyAzA[one]CxA[second-lower]");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "second-lower");
	free(result);

	strcpy(buffer2, "(xCyA.CzA[word-boundary] more");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "word-boundary");
	free(result);

	strcpy(buffer2, "no:CA[one] (CA[after-brace]");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_str_eq(result, "after-brace");
	free(result);
END_TEST


START_TEST (test_no_encoding_specified)
	char *result;
	char buffer[] = "you're not gonna find it";
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);

	result = DetectEncoding(sgfc);
	ck_assert_ptr_eq(result, NULL);

	strcpy(buffer, "you're not gonna CA[it");
	sgfc->b_end = buffer + strlen(buffer);
	result = DetectEncoding(sgfc);
	ck_assert_ptr_eq(result, NULL);
END_TEST


START_TEST (test_unknown_encoding)
	char *result;

//	result = ConvertEncoding(sgfc, "none-of-your-business", NULL);
	ck_assert_ptr_eq(result, NULL);
END_TEST


START_TEST (test_basic_conversion)
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
	sgfc->buffer = dst_buffer;
	sgfc->b_end = dst_pos;

	/* ... and convert text back */
//	result = ConvertEncoding(sgfc, "UTF-16LE", NULL);
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, src_buffer);
	free(result);
END_TEST


START_TEST (test_bad_char_conversion)
	char *result;
	char buffer[50]; /* large enough so that no reallocation is necessary */
	memset(buffer, 0xFF, 50);
	/* 01234567890123456789012345; fill rest with \xFF; no \0! */
	strncpy(buffer, "simple test with bad chars", 26);
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + 50;
	buffer[3] = '\xF0';		/* single bad byte */
	buffer[8] = '\xC0';		/* 2 byte seq without second byte */
	buffer[13] = '\xE2';	/* 3 byte seq without third byte */
	buffer[14] = '\x82';
	buffer[18] = '\x81';	/* single continuation byte */

	/* UTF-8 -> UTF-8 eliminates bad chars */
//	result = ConvertEncoding(sgfc, "UTF-8", NULL);
	ck_assert_ptr_ne(result, NULL);
	/* Test assumes UTF-8 encoded string literals */
	ck_assert_str_eq(result, "sim\uFFFDle t\uFFFDst w\uFFFDh b\uFFFDd chars\uFFFD");
	free(result);
END_TEST


START_TEST (test_buffer_overflow_conversion)
	char *result;

	char buffer[2] = {'\xE4', 0}; /* "ä" in ISO-8859-1 */
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + 1;
//	result = ConvertEncoding(sgfc, "ISO-8859-1", NULL);
	ck_assert_ptr_ne(result, NULL);
	ck_assert_str_eq(result, "\u00E4");
	free(result);

	char buffer2[1000];
	memset(buffer2, ' ', 950);		/* no expansion first */
	memset(buffer2+950, 0xE4, 50);	/* then double expansions */
	sgfc->buffer = buffer2;			/* -> bad case for calculation of "needed" bytes */
	sgfc->b_end = buffer2 + 1000;
//	result = ConvertEncoding(sgfc, "ISO-8859-1", NULL);
	ck_assert_ptr_ne(result, NULL);
	char expected[1051];
	memset(expected, ' ', 950);
	for(int i=0; i < 50; i++) {
		expected[950+i*2]	= '\xC3';	/* 2 byte UTF-8 encoding of 'ä' */
		expected[950+i*2+1] = '\xA4';
	}
	expected[1050] = 0;
	ck_assert_str_eq(result, expected);
	free(result);
END_TEST



TCase *sgfc_tc_encoding(void)
{
	TCase *tc;

	tc = tcase_create("encoding");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_detect_encoding_BOM);
	tcase_add_test(tc, test_detect_encoding);
	tcase_add_test(tc, test_no_encoding_specified);
//	tcase_add_test(tc, test_unknown_encoding);
//	tcase_add_test(tc, test_basic_conversion);
//	tcase_add_test(tc, test_bad_char_conversion);
//	tcase_add_test(tc, test_buffer_overflow_conversion);
	return tc;
}
