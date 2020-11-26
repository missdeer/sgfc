/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

struct PropValue *prop_value;

void parse_text_setup(void)
{
	common_setup();
	SaveMalloc(struct PropValue *, prop_value, sizeof(struct PropValue), "propval")
	memset(prop_value, 0, sizeof(struct PropValue));
	prop_value->row = 3;
	prop_value->col = 1;
}

void parse_text_teardown(void)
{
	common_teardown();
	free(prop_value);
}

START_TEST (test_basic_string)
    char text[] = "basic test";
	prop_value->value = text;
	prop_value->value_len = strlen(text);
    int len = Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_int_eq(len, 10);
	ck_assert_str_eq(text, "basic test");
END_TEST


START_TEST (test_soft_linebreak)
    char text[] = "soft\\\nbreak";
	prop_value->value = text;
	prop_value->value_len = strlen(text);
	int len = Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_str_eq(text, "softbreak");

    char text2[] = "soft\\\nbreak:SIMPLE";
	prop_value->value2 = text2;
	prop_value->value2_len = strlen(text2);
	Parse_Text(sgfc, prop_value, 2, PVT_SIMPLE);
	ck_assert_str_eq(text2, "softbreak:SIMPLE");
END_TEST


START_TEST (test_trailing_spaces)
    char text[] = "trailing   ";
	prop_value->value = text;
	prop_value->value_len = strlen(text);
	Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_str_eq(text, "trailing");

    char text2[] = "trailing2 \\ \\ ";
	prop_value->value = text2;
	prop_value->value_len = strlen(text2);
	Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_str_eq(text2, "trailing2");

    char text3[] = "trailing3 \\\\ ";
	prop_value->value = text3;
	prop_value->value_len = strlen(text3);
	Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_str_eq(text3, "trailing3 \\");

    char text4[] = "trailing4 \\\\\\ ";
	prop_value->value = text4;
	prop_value->value_len = strlen(text4);
	Parse_Text(sgfc, prop_value, 1, 0);
	ck_assert_str_eq(text4, "trailing4 \\");
END_TEST


START_TEST (test_trailing_spaces_simpletext)
    char text[] = "trailing \n ";
	prop_value->value = text;
	prop_value->value_len = strlen(text);
	Parse_Text(sgfc, prop_value, 1, PVT_SIMPLE);
	ck_assert_str_eq(text, "trailing");

    char text2[] = "trailing2 \n\n\\ \\\n ";
	prop_value->value = text2;
	prop_value->value_len = strlen(text2);
	Parse_Text(sgfc, prop_value, 1, PVT_SIMPLE);
	ck_assert_str_eq(text2, "trailing2");

    char text3[] = "trailing3 \r\n\\\r\\\n\n\r ";
	prop_value->value = text3;
	prop_value->value_len = strlen(text3);
	Parse_Text(sgfc, prop_value, 1, PVT_SIMPLE);
	ck_assert_str_eq(text3, "trailing3");
END_TEST


START_TEST (test_composed_simpletext_linebreaks)
	struct Property p;
	struct PropValue v;

	p.flags = PVT_SIMPLE|PVT_COMPOSE;
	p.value = &v;

	char val1a[] = "aaa \\ ", val2a[] = "bbb \\ ";
	v.value = val1a;	v.value_len = strlen(val1a);
	v.value2 = val2a;	v.value2_len = strlen(val2a);
    Check_Text(sgfc, &p, &v);
	ck_assert_str_eq(val1a, "aaa");
	ck_assert_str_eq(val2a, "bbb");

	char val1b[] = "a\\\naa", val2b[] = "b\\\nbb";
	v.value = val1b;	v.value_len = strlen(val1b);
	v.value2 = val2b;	v.value2_len = strlen(val2b);
    Check_Text(sgfc, &p, &v);
	ck_assert_str_eq(val1b, "aaa");
	ck_assert_str_eq(val2b, "bbb");

	char val1c[] = "aa\na", val2c[] = "bb\nb";
	v.value = val1c;	v.value_len = strlen(val1c);
	v.value2 = val2c;	v.value2_len = strlen(val2c);
    Check_Text(sgfc, &p, &v);
	ck_assert_str_eq(val1c, "aa a");
	ck_assert_str_eq(val2c, "bb b");
END_TEST


TCase *sgfc_tc_parse_text(void)
{
	TCase *tc;

	tc = tcase_create("parse_text");
	tcase_add_checked_fixture(tc, parse_text_setup, parse_text_teardown);

	tcase_add_test(tc, test_basic_string);
	tcase_add_test(tc, test_soft_linebreak);
	tcase_add_test(tc, test_trailing_spaces);
	tcase_add_test(tc, test_trailing_spaces_simpletext);
	tcase_add_test(tc, test_composed_simpletext_linebreaks);
	return tc;
}
