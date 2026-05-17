/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/gameinfo-correction.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>

#include "test-common.h"


static void AssertGameInfoValue(token id, char *idstr, const char *input, const char *expected)
{
	struct PropValue v = { .value = SafeDupString(input, "value"), .value_len = strlen(input) };
	struct Property p = { .id = id, .idstr = idstr, .value = &v };

	bool result = Check_GameInfo(sgfc, &p, &v);

	ck_assert(result);
	ck_assert_str_eq(v.value, expected);
	ck_assert_uint_eq(v.value_len, strlen(expected));

	free(v.value);
}

static void AssertGameInfoValueUnchanged(token id, char *idstr, const char *input)
{
	AssertGameInfoValue(id, idstr, input, input);
}


START_TEST (test_result_values)
{
	struct {
		const char *input;
		const char *expected;
	} cases[] = {
		{ "0", "0" },
		{ "0.5", "0" },
		{ "?", "?" },
		{ "Draw", "Draw" },
		{ "draw", "Draw" },
		{ "Jigo", "Draw" },
		{ "Void", "Void" },
		{ "void", "Void" },
		{ "B+", "B+" },
		{ "W+42", "W+42" },
		{ "B+42.3", "B+42.3" },
		{ "w+42.", "W+42" },
		{ "B+.3", "B+0.3" },
		{ "B+R", "B+R" },
		{ "w+r", "W+R" },
		{ "B+Resign", "B+Resign" },
		{ "B+Res", "B+R" },
		{ "W+T", "W+T" },
		{ "B+t", "B+T" },
		{ "W+Time", "W+Time" },
		{ "W+F", "W+F" },
		{ "B+f", "B+F" },
		{ "B+Forfeit", "B+Forfeit" },
		{ "Black resigns", "W+R" },
		{ "White wins by 3 1/2 points", "W+3.5" },
		{ "Black wins by resignation", "B+R" },
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
		AssertGameInfoValue(TKN_RE, "RE", cases[i].input, cases[i].expected);
}
END_TEST


START_TEST (test_result_invalid_values)
{
	const char *cases[] = {
		"",
		"Black",
		"1",
		"No key words here",
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
		AssertGameInfoValueUnchanged(TKN_RE, "RE", cases[i]);
}
END_TEST


START_TEST (test_date_values)
{
	struct {
		const char *input;
		const char *expected;
	} cases[] = {
		{ "2020-12-31", "2020-12-31" },
		{ "2020-12", "2020-12" },
		{ "2020", "2020" },
		{ "2020-01-01,04-15", "2020-01-01,04-15" },
		{ "2020-01-01,04", "2020-01-01,04" },
		{ "2020-01,04-15", "2020-01,04-15" },
		{ "2020-01,04", "2020-01,04" },
		{ "2020-01-01,04-15,22", "2020-01-01,04-15,22" },
		{ "2019,2020-10-22", "2019,2020-10-22" },
		{ "1996-01-01,04-15,2021-10-22", "1996-01-01,04-15,2021-10-22" },
		{ "2019-01-31", "2019-01-31" },
		{ "2019-02-28", "2019-02-28" },
		{ "2020-02-29", "2020-02-29" },
		{ "1600-02-29", "1600-02-29" },
		{ "Dec 31 2020", "2020-12-31" },
		{ "31-12-2020", "2020-12-31" },
		{ "2020/12/31", "2020-12-31" },
		{ "74/31/10", "1974-10-31" },
		{ "2020 12th 31st", "2020-12-31" },
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
		AssertGameInfoValue(TKN_DT, "DT", cases[i].input, cases[i].expected);
}
END_TEST


START_TEST (test_date_invalid_values)
{
	const char *cases[] = {
		"",
		"Some time ago",
		"弘化3年7月21日",
		"2020-13-01",
		"2019-02-29",
		"2019-04-31",
		"2100-02-29",
		"2020-00-01",
		"0000-00-00",
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
		AssertGameInfoValueUnchanged(TKN_DT, "DT", cases[i]);
}
END_TEST


TCase *sgfc_tc_gameinfo_correction(void)
{
	TCase *tc;

	tc = tcase_create("gameinfo_correction");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_result_values);
	tcase_add_test(tc, test_result_invalid_values);
	tcase_add_test(tc, test_date_values);
	tcase_add_test(tc, test_date_invalid_values);
	return tc;
}
