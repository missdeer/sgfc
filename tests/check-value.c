/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/check-value.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"


START_TEST (test_composed_value_check)
{
	struct Property p;
	struct PropValue v;

	char val1a[] = "10", val2a[] = "11";
	v.value = val1a;
	v.value2 = val2a;
	int result = Check_Value(sgfc, &p, &v, PVT_COMPOSE, Parse_Number);
	ck_assert_int_eq(result, TRUE);
	ck_assert_str_eq(v.value, "10");
	ck_assert_str_eq(v.value2, "11");

	char val1b[] = "x1y0z", val2b[] = " a1b1c ";
	v.value = val1b;
	v.value2 = val2b;
	result = Check_Value(sgfc, &p, &v, PVT_COMPOSE, Parse_Number);
	ck_assert_int_eq(result, TRUE);
	ck_assert_str_eq(v.value, "10");
	ck_assert_str_eq(v.value2, "11");
}
END_TEST


START_TEST (test_composed_value_removed)
{
	struct Property p;
	struct PropValue v;

	char val1a[] = "foo", val2a[] = "11";
	v.value = val1a;
	v.value2 = val2a;
	int result = Check_Value(sgfc, &p, &v, PVT_COMPOSE, Parse_Number);
	ck_assert_int_eq(result, FALSE);

	char val1b[] = "10", val2b[] = "foo";
	v.value = val1b;
	v.value2 = val2b;
	result = Check_Value(sgfc, &p, &v, PVT_COMPOSE, Parse_Number);
	ck_assert_int_eq(result, FALSE);
}
END_TEST


TCase *sgfc_tc_check_value()
{
	TCase *tc;

	tc = tcase_create("check_value");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_composed_value_check);
	tcase_add_test(tc, test_composed_value_removed);
	return tc;
}
