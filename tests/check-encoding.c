/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/check-encoding.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iconv.h>
#include <check.h>

#include "test-common.h"

static bool encoding_fallback_seen;


static bool encoding_error_handler(uint32_t type, struct SGFInfo *sgfi, va_list arglist)
{
	(void)sgfi;
	(void)arglist;
	if(type == WS_ENCODING_FALLBACK)
		encoding_fallback_seen = true;
	return true;
}


static bool LoadParseBuffer(char *buffer, size_t size)
{
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + size;
	ck_assert_int_eq(LoadSGFFromFileBuffer(sgfc), true);
	return ParseSGF(sgfc);
}


static char *CallDetectEncoding(char *buffer, size_t size, enum encoding_source *source)
{
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + size;
	return DetectEncoding(sgfc, source);
}


static void AssertRootComment(const char *expected)
{
	struct Property *comment = FindProperty(sgfc->root, TKN_C);
	ck_assert_ptr_nonnull(comment);
	ck_assert_ptr_nonnull(comment->value);
	ck_assert_str_eq(comment->value->value, expected);
}


START_TEST (test_detect_encoding_BOM)
{
	char *result;
	enum encoding_source source;
	char buffer[4] = {'\xFE', '\xFF', ' ', ' '};

	result = CallDetectEncoding(buffer, 4, &source);
	ck_assert_str_eq(result, "UTF-16BE");
	ck_assert_int_eq(source, ENCODING_SOURCE_BOM);
	free(result);

	buffer[0] = '\xFF';
	buffer[1] = '\xFE';
	result = CallDetectEncoding(buffer, 4, &source);
	ck_assert_str_eq(result, "UTF-16LE");
	ck_assert_int_eq(source, ENCODING_SOURCE_BOM);
	free(result);

	buffer[2] = 0;
	buffer[3] = 0;
	result = CallDetectEncoding(buffer, 4, &source);
	ck_assert_str_eq(result, "UTF-32LE");
	ck_assert_int_eq(source, ENCODING_SOURCE_BOM);
	free(result);

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = '\xFE';
	buffer[3] = '\xFF';
	result = CallDetectEncoding(buffer, 4, &source);
	ck_assert_str_eq(result, "UTF-32BE");
	ck_assert_int_eq(source, ENCODING_SOURCE_BOM);
	free(result);

	buffer[0] = '\xEF';
	buffer[1] = '\xBB';
	buffer[2] = '\xBF';
	buffer[3] = '\n';
	result = CallDetectEncoding(buffer, 4, &source);
	ck_assert_str_eq(result, "UTF-8");
	ck_assert_int_eq(source, ENCODING_SOURCE_BOM);
	free(result);
}
END_TEST


START_TEST (test_detect_encoding_limits)
{
	char nested[] = "(((CA[UTF-8]))";
	enum encoding_source source;
	char *result = CallDetectEncoding(nested, strlen(nested), &source);
	ck_assert_ptr_eq(result, NULL);
	ck_assert_int_eq(source, ENCODING_SOURCE_NONE);

	char delayed[DEFAULT_ENCODING_DETECT_SCAN_LIMIT + 200];
	memset(delayed, 'a', sizeof(delayed));
	delayed[0] = '(';
	memcpy(delayed + DEFAULT_ENCODING_DETECT_SCAN_LIMIT + 5, "CA[UTF-8]", 9);
	delayed[sizeof(delayed)-1] = 0;
	result = CallDetectEncoding(delayed, sizeof(delayed) - 1, &source);
	ck_assert_ptr_eq(result, NULL);
	ck_assert_int_eq(source, ENCODING_SOURCE_NONE);
}
END_TEST


START_TEST (test_detect_encoding)
{
	char *result;
	enum encoding_source source;

	char buffer[] = "some (text CA[basic-case] more text";
	result = CallDetectEncoding(buffer, strlen(buffer), &source);
	ck_assert_str_eq(result, "basic-case");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer2[] = "some (CA\n [ spaces \n] ";
	result = CallDetectEncoding(buffer2, strlen(buffer2), &source);
	ck_assert_str_eq(result, "spaces");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer3[] = "some text in (front ClowerAcase\n [ lower-case]";
	result = CallDetectEncoding(buffer3, strlen(buffer3), &source);
	ck_assert_str_eq(result, "lower-case");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer4[] = "(CCA[one]CA[second]";
	result = CallDetectEncoding(buffer4, strlen(buffer4), &source);
	ck_assert_str_eq(result, "second");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer5[] = "(xCyAzA[one]CxA[second-lower]";
	result = CallDetectEncoding(buffer5, strlen(buffer5), &source);
	ck_assert_str_eq(result, "second-lower");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer6[] = "(xCyA.CzA[word-boundary] more";
	result = CallDetectEncoding(buffer6, strlen(buffer6), &source);
	ck_assert_str_eq(result, "word-boundary");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);

	char buffer7[] = "no:CA[one] (CA[after-brace]";
	result = CallDetectEncoding(buffer7, strlen(buffer7), &source);
	ck_assert_str_eq(result, "after-brace");
	ck_assert_int_eq(source, ENCODING_SOURCE_CA);
	free(result);
}
END_TEST


START_TEST (test_no_encoding_specified)
{
	char *result;
	enum encoding_source source;

	char buffer[] = "you're not gonna find it";
	result = CallDetectEncoding(buffer, strlen(buffer), &source);
	ck_assert_ptr_eq(result, NULL);
	ck_assert_int_eq(source, ENCODING_SOURCE_NONE);

	char buffer2[] = "you're not gonna CA[it";
	result = CallDetectEncoding(buffer2, strlen(buffer2), &source);
	ck_assert_ptr_eq(result, NULL);
	ck_assert_int_eq(source, ENCODING_SOURCE_NONE);

	char buffer3[] = "(;CA[])";
	result = CallDetectEncoding(buffer3, strlen(buffer3), &source);
	ck_assert_ptr_eq(result, NULL);
	ck_assert_int_eq(source, ENCODING_SOURCE_NONE);

	char tiny[] = "abc";
	for(size_t len = 0; len < 4; len++)
	{
		result = CallDetectEncoding(tiny, len, &source);
		ck_assert_ptr_eq(result, NULL);
		ck_assert_int_eq(source, ENCODING_SOURCE_NONE);
	}
}
END_TEST


START_TEST (test_forced_encoding_overrides_ca)
{
	char buffer[] = "(;FF[4]CA[EUC-CN]C[\x99])";

	sgfc->options->forced_encoding = "Windows-1252";
	ck_assert_int_eq(LoadParseBuffer(buffer, sizeof(buffer)), true);
	AssertRootComment("\xE2\x84\xA2");
}
END_TEST


START_TEST (test_default_encoding_used_without_ca)
{
	char buffer[] = "(;FF[4]C[\xE1\xD9])";

	sgfc->options->default_encoding = "ISO-8859-7";
	ck_assert_int_eq(LoadParseBuffer(buffer, sizeof(buffer)), true);
	AssertRootComment("\xCE\xB1\xCE\xA9");
}
END_TEST


START_TEST (test_load_big5_escaped_skeleton_byte_with_e1)
{
	/* 5D == ']' */
	char buffer[] = "(;FF[4]CA[Big5]C[\xA6\x5D])";

	sgfc->options->encoding = OPTION_ENCODING_EVERYTHING;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), true);
	AssertRootComment("\xE5\x9B\xA0");
}
END_TEST


START_TEST (test_load_sjis_escaped_skeleton_byte_with_e1)
{
	/* 5C == '\' */
	char buffer[] = "(;FF[4]CA[SJIS]C[\x90\x5C])";

	sgfc->options->encoding = OPTION_ENCODING_EVERYTHING;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), true);
	AssertRootComment("\xE7\x94\xB3");
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
	cd = iconv_open("UTF-8", "UTF-16LE");
	result = DecodeBuffer(sgfc, cd, dst_buffer, (size_t)(dst_pos - dst_buffer), 0, NULL);
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

START_TEST (test_open_iconv_fallback)
{
	encoding_fallback_seen = false;
	print_error_handler = encoding_error_handler;
	sgfc->options->forced_encoding = NULL;
	sgfc->options->default_encoding = "UTF-8";

	const char *encoding_name = NULL;
	iconv_t cd = OpenIconV(sgfc, "NOT-A-REAL-ENCODING-123", &encoding_name);
	ck_assert_ptr_ne(cd, NULL);
	ck_assert_str_eq(encoding_name, "UTF-8");
	ck_assert(encoding_fallback_seen);
	iconv_close(cd);
}
END_TEST


START_TEST (test_open_iconv_forced_encoding)
{
	encoding_fallback_seen = false;
	print_error_handler = encoding_error_handler;
	sgfc->options->forced_encoding = "UTF-8";
	sgfc->options->default_encoding = "ISO-8859-1";

	const char *encoding_name = NULL;
	iconv_t cd = OpenIconV(sgfc, "NOT-A-REAL-ENCODING-123", &encoding_name);
	ck_assert_ptr_ne(cd, NULL);
	ck_assert_str_eq(encoding_name, "UTF-8");
	ck_assert(!encoding_fallback_seen);
	iconv_close(cd);
}
END_TEST


TCase *sgfc_tc_encoding(void)
{
	TCase *tc;

	tc = tcase_create("encoding");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_detect_encoding_BOM);
	tcase_add_test(tc, test_detect_encoding);
	tcase_add_test(tc, test_detect_encoding_limits);
	tcase_add_test(tc, test_no_encoding_specified);
	tcase_add_test(tc, test_forced_encoding_overrides_ca);
	tcase_add_test(tc, test_default_encoding_used_without_ca);
	tcase_add_test(tc, test_load_big5_escaped_skeleton_byte_with_e1);
	tcase_add_test(tc, test_load_sjis_escaped_skeleton_byte_with_e1);
	tcase_add_test(tc, test_basic_conversion);
	tcase_add_test(tc, test_bad_char_conversion);
	tcase_add_test(tc, test_buffer_overflow_conversion);
	tcase_add_test(tc, test_8bit_value_in_middle);
	tcase_add_test(tc, test_8bit_value_at_end);
	tcase_add_test(tc, test_open_iconv_fallback);
	tcase_add_test(tc, test_open_iconv_forced_encoding);
	return tc;
}


/**************************************************************************
*** Test Case 2: also verifies detected errors
**************************************************************************/


static void verifying_setup(void)
{
	common_setup();

	print_error_handler = verifying_error_handler;
	allowed_errors = NULL;
	expected_error_occurred = false;
}


START_TEST (test_load_utf8_bom)
{
	char buffer[] = "\xEF\xBB\xBF(;C[s\xC3\xBC\xC3\x9F])";

	expected_error = E_NO_ERROR;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), true);
	ck_assert(expected_error_occurred);
	AssertRootComment("süß");
}
END_TEST


START_TEST (test_load_utf16be_bom)
{
	char buffer[] = {
		'\xFE', '\xFF',
		0x00, '(', 0x00, ';', 0x00, 'F', 0x00, 'F', 0x00, '[', 0x00, '4', 0x00, ']',
		0x00, 'C', 0x00, 'A', 0x00, '[', 0x00, 'U', 0x00, 'T', 0x00, 'F', 0x00, '-',
		0x00, '1', 0x00, '6', 0x00, 'B', 0x00, 'E', 0x00, ']', 0x00, 'C', 0x00, '[',
		0x00, 'B', 0x00, 'E', 0x00, '!', 0x00, ']', 0x00, ')'
	};

	expected_error = E_NO_ERROR;
	ck_assert_int_eq(LoadParseBuffer(buffer, sizeof(buffer)), true);
	ck_assert(expected_error_occurred);
	AssertRootComment("BE!");
}
END_TEST


START_TEST (test_load_utf16le_bom)
{
	char buffer[] = {
		'\xFF', '\xFE', 
		'(', 0x00, ';', 0x00, 'F', 0x00, 'F', 0x00, '[', 0x00, '4', 0x00, ']', 0x00,
		'C', 0x00, 'A', 0x00, '[', 0x00, 'U', 0x00, 'T', 0x00, 'F', 0x00, '-', 0x00,
		'1', 0x00, '6', 0x00, 'L', 0x00, 'E', 0x00, ']', 0x00, 'C', 0x00, '[', 0x00,
		'L', 0x00, 'E', 0x00, '!', 0x00, ']', 0x00, ')', 0x00
	};

	expected_error = E_NO_ERROR;
	ck_assert_int_eq(LoadParseBuffer(buffer, sizeof(buffer)), true);
	ck_assert(expected_error_occurred);
	AssertRootComment("LE!");
}
END_TEST


START_TEST (test_e1_rejects_utf8_bom_ca_mismatch)
{
	char buffer[] = "\xEF\xBB\xBF(;FF[4]CA[ISO-8859-15]C[s\xC3\xBC\xC3\x9F])";

	expected_error = FE_WRONG_ENCODING;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), false);
	ck_assert(expected_error_occurred);
}
END_TEST


START_TEST (test_utf16_without_bom_not_detected)
{
	char buffer[] = {
		0x00, '(', 0x00, ';', 0x00, 'F', 0x00, 'F', 0x00, '[', 0x00, '4', 0x00, ']',
		0x00, 'C', 0x00, 'A', 0x00, '[', 0x00, 'U', 0x00, 'T', 0x00, 'F', 0x00, '-',
		0x00, '1', 0x00, '6', 0x00, 'B', 0x00, 'E', 0x00, ']', 0x00, 'C', 0x00, '[',
		0x00, 'N', 0x00, 'o', 0x00, '!', 0x00, ']', 0x00, ')'
	};
	uint32_t allowed[] = {
		E_ILLEGAL_OUTSIDE_CHAR, E_NO_PROP_VALUES, WS_UNKNOWN_PROPERTY,
		E_MISSING_SEMICOLON, 0
	};

	expected_error = W_CTRL_BYTE_DELETED;
	allowed_errors = allowed;
	ck_assert_int_eq(LoadParseBuffer(buffer, sizeof(buffer)), true);
	ck_assert(expected_error_occurred);
}
END_TEST


START_TEST (test_e1_rejects_multiple_encodings)
{
	char buffer[] = "(;FF[4]CA[UTF-8]C[one])(;FF[4]CA[ISO-8859-15]C[two])";

	sgfc->options->encoding = OPTION_ENCODING_EVERYTHING;
	expected_error = FE_MULTIPLE_ENCODINGS;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), false);
	ck_assert_int_eq(expected_error_occurred, true);
}
END_TEST


START_TEST (test_e2_warns_for_multiple_encodings)
{
	char buffer[] = "(;FF[4]CA[UTF-8]C[one])(;FF[4]CA[ISO-8859-1]C[two])";

	sgfc->options->encoding = OPTION_ENCODING_TEXT_ONLY;
	expected_error = WS_CA_DIFFERS;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), true);
	ck_assert_int_eq(expected_error_occurred, true);
}
END_TEST


START_TEST (test_load_big5_skeleton_byte_fails_with_e2)
{
	/* 5D == ']' would need to be escaped within multibyte */
	char buffer[] = "(;FF[4]CA[Big5]C[\xA6\x5D])";
	uint32_t allowed[] = { E_ILLEGAL_OUTSIDE_CHAR, 0 };

	expected_error = WS_ENCODING_ERRORS;
	allowed_errors = allowed;
	sgfc->options->encoding = OPTION_ENCODING_TEXT_ONLY;
	ck_assert_int_eq(LoadParseBuffer(buffer, strlen(buffer)), true);
	ck_assert(expected_error_occurred);
}
END_TEST


TCase *sgfc_tc_encoding2(void)
{
	TCase *tc;

	tc = tcase_create("encoding2");
	tcase_add_checked_fixture(tc, verifying_setup, common_teardown);

	tcase_add_test(tc, test_load_utf8_bom);
	tcase_add_test(tc, test_load_utf16be_bom);
	tcase_add_test(tc, test_load_utf16le_bom);
	tcase_add_test(tc, test_utf16_without_bom_not_detected);
	tcase_add_test(tc, test_e1_rejects_utf8_bom_ca_mismatch);
	tcase_add_test(tc, test_e1_rejects_multiple_encodings);
	tcase_add_test(tc, test_e2_warns_for_multiple_encodings);
	tcase_add_test(tc, test_load_big5_skeleton_byte_fails_with_e2);
	return tc;
}
