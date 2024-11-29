/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/position.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"
#include <stdbool.h>


void VerifyTreeValueLength(struct Node *n, int phase)
{
	while(n)
	{
		struct Property *p = n->prop;
		while(p)
		{
			struct PropValue *v = p->value;
			while(v)
			{
				/* brittle test: row=22, col=2: prop value with \00 byte, still present after load */
				ck_assert_msg(strlen(v->value) == v->value_len || (phase == 1 && v->row == 22 && v->col == 2),
				  			  "phase %d: %s_v1 at %ld:%ld, strlen=%ld != value_len=%ld",
				   			  phase, p->idstr, v->row, v->col, strlen(v->value), v->value_len);
				if(v->value2)
					ck_assert_msg(strlen(v->value2) == v->value2_len,
								  "phase %d: %s_v2 at %ld:%ld, strlen=%ld != value_len=%ld",
								  phase, p->idstr, v->row, v->col, strlen(v->value2), v->value2_len);
				v = v->next;
			}
			p = p->next;
		}
		if(n->sibling)
			VerifyTreeValueLength(n->sibling, phase);
		n = n->child;
	}
}

START_TEST (test_length_with_test_sgf)
{
	int ret = LoadSGF(sgfc, "../test-files/test.sgf");
	ck_assert_int_eq(ret, true);
	VerifyTreeValueLength(sgfc->root, 1);

	ParseSGF(sgfc);
	VerifyTreeValueLength(sgfc->root, 2);
	free(sgfc->buffer);		/* common_teardown doesn't free buffer */
}
END_TEST


TCase *sgfc_tc_value_length(void)
{
	TCase *tc;

	tc = tcase_create("test-files");
	tcase_add_checked_fixture(tc, common_setup, common_teardown);

	tcase_add_test(tc, test_length_with_test_sgf);
	return tc;
}
