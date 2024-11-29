/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 tests/parse-text.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include "test-common.h"

static U_LONG expected_error;
static U_LONG allowed_error;	/* additional error that might occur */
static bool expected_error_occurred;


static bool mock_error_handler(U_LONG type, struct SGFInfo *sgfi, va_list arglist)
{
	if(type == expected_error)
		expected_error_occurred = true;
	else if(type != E_NO_ERROR && type != allowed_error)
	{
		ck_assert_msg(type == expected_error, "expected error: %ld (%lx); received: %ld (%lx)",
					  expected_error & M_ERROR_NUM, expected_error, type & M_ERROR_NUM, type);
	}
	return true;
}


static void setup(void)
{
	common_setup();
	print_error_handler = mock_error_handler;
	expected_error_occurred = false;
	allowed_error = E_NO_ERROR;
}


static void trigger_error(U_LONG type, char *buffer, char *expected)
{
	sgfc->buffer = buffer;
	sgfc->b_end = buffer + strlen(buffer);
	expected_output = expected;
	expected_error = type;
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, true);
	ParseSGF(sgfc);
	ret = SaveSGF(sgfc, SetupSaveTestIO, "outfile");
	ck_assert_int_eq(ret, true);
	ck_assert(expected_error_occurred);
}


START_TEST (test_W_SGF_IN_HEADER)
{
	trigger_error(W_SGF_IN_HEADER,
				  "B[aa](;)",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_FE_NO_SGFDATA)
{
	char sgf[] = "no data";
	sgfc->buffer = sgf;
	sgfc->b_end = sgf + strlen(sgf);
	expected_error = FE_NO_SGFDATA;
	int ret = LoadSGFFromFileBuffer(sgfc);
	ck_assert_int_eq(ret, false);
}
END_TEST


START_TEST (test_E_ILLEGAL_OUTSIDE_CHARS)
{
	trigger_error(E_ILLEGAL_OUTSIDE_CHARS,
				  "(; illegal )",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_VARIATION_NESTING)
{
	trigger_error(E_VARIATION_NESTING,
				  "(;B[aa]",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[aa])\n");
}
END_TEST


START_TEST (test_E_UNEXPECTED_EOF)
{
	trigger_error(E_UNEXPECTED_EOF,
				  "(;B[aa",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_PROPID_TOO_LONG)
{
	trigger_error(E_PROPID_TOO_LONG,
				  "(;XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX[])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_EMPTY_VARIATION)
{
	trigger_error(E_EMPTY_VARIATION,
				  "(;;B[aa]()(;W[bb]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[aa];W[bb])\n");
}
END_TEST


START_TEST (test_E_TOO_MANY_VALUES)
{
	trigger_error(E_TOO_MANY_VALUES,
				  "(;;B[aa][bb])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[aa])\n");
}
END_TEST


START_TEST (test_E_BAD_VALUE_DELETED)
{
	trigger_error(E_BAD_VALUE_DELETED,
				  "(;B[111];PL[r])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];)\n");
}
END_TEST


START_TEST (test_E_BAD_VALUE_CORRECTED)
{
	trigger_error(E_BAD_VALUE_CORRECTED,
				  "(;FF[4];B[a a];DM[1  kk]BL[30.])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[aa];DM[1]BL[30])\n");
}
END_TEST


START_TEST (test_E_LC_IN_PROPID)
{
	trigger_error(E_LC_IN_PROPID,
				  "(;FF[4];Black[cc];White[dd])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[cc];W[dd])\n");
}
END_TEST


START_TEST (test_E_EMPTY_VALUE_DELETED)
{
	trigger_error(E_EMPTY_VALUE_DELETED,
				  "(;PL[]AB[])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_W_EMPTY_VALUE_DELETED)
{
	trigger_error(W_EMPTY_VALUE_DELETED,
				  "(;C[])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_BAD_ROOT_PROP)
{
	trigger_error(E_BAD_ROOT_PROP,
				  "(;FF[four]GM[Go]SZ[-12])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_WCS_GAME_NOT_GO)
{
	trigger_error(WCS_GAME_NOT_GO,
				  "(;GM[12];B[weird move])",
				  "(;FF[4]CA[UTF-8]GM[12];B[weird move])\n");
}
END_TEST


START_TEST (test_E_NO_PROP_VALUES)
{
	trigger_error(E_NO_PROP_VALUES,
				  "(;B[cc]PL;W[aa];AB;B[ab] G C[game])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[cc];W[aa];;B[ab]C[game])\n");
}
END_TEST


START_TEST (test_E_VARIATION_START)
{
	trigger_error(E_VARIATION_START,
				  "(;B[cc]((;W[dd]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[cc];W[dd])\n");
}
END_TEST


START_TEST (test_E_COMPOSE_EXPECTED)
{
	trigger_error(E_COMPOSE_EXPECTED,
				  "(;FF[4];LB[aa][bb][cc])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];)\n");
}
END_TEST


START_TEST (test_WS_MOVE_IN_ROOT)
{
	sgfc->options->fix_variation = true;
	trigger_error(WS_MOVE_IN_ROOT,
				  "(;B[aa])(;W[bb])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[aa])\n"
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];W[bb])\n");
}
END_TEST


START_TEST (test_E_BAD_COMPOSE_CORRECTED)
{
	trigger_error(E_BAD_COMPOSE_CORRECTED,
				  "(;FF[4];LB[a a : text])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];LB[aa: text])\n");
}
END_TEST


START_TEST (test_E_DOUBLE_PROP)
{
	trigger_error(E_DOUBLE_PROP,
				  "(;FF[4]LB[aa:1]C[x]LB[bb:2]C[y];W[aa]W[bb])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]LB[aa:1][bb:2]C[x\n\ny];W[aa])\n");
}
END_TEST


START_TEST (test_W_PROPERTY_DELETED)
{
	sgfc->options->keep_obsolete_props = false;
	trigger_error(W_PROPERTY_DELETED,
				  "(;FF[1]BS[1]RG[aa][cc])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E4_MOVE_SETUP_MIXED)
{
	trigger_error(E4_MOVE_SETUP_MIXED,
				  "(;;B[cc]AW[dd]) (;;B[cc]PL[B])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];AW[dd];B[cc])\n"
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[cc])\n"); /* single PL gets deleted */
}
END_TEST


START_TEST (test_WS_LONG_PROPID)
{
	allowed_error = WS_UNKNOWN_PROPERTY;
	trigger_error(WS_LONG_PROPID,
				  "(;PIW[])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]PIW[])\n");
}
END_TEST


START_TEST (test_E_ROOTP_NOT_IN_ROOTN)
{
	trigger_error(E_ROOTP_NOT_IN_ROOTN,
				  "(;B[aa];GM[1])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[aa];)\n");
}
END_TEST


START_TEST (test_E4_FAULTY_GC)
{
	trigger_error(E4_FAULTY_GC,
				  "(;RE[Someone wins])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nRE[Someone wins]\n\n)\n");
}
END_TEST


START_TEST (test_WS_UNKNOWN_PROPERTY)
{
	trigger_error(WS_UNKNOWN_PROPERTY,
				  "(;KK[txt])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]KK[txt])\n");
}
END_TEST


START_TEST (test_E_MISSING_SEMICOLON)
{
	char sgf[] = "( GM[1]FF[3])";
	trigger_error(E_MISSING_SEMICOLON,
				  sgf, /* necessary, because FindStart() modifies buffer */
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E_TWO_MOVES_IN_NODE)
{
	trigger_error(E_TWO_MOVES_IN_NODE,
				  "(;B[cc]W[dd])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[cc];W[dd])\n");
}
END_TEST


START_TEST (test_E_POSITION_NOT_UNIQUE)
{
	trigger_error(E_POSITION_NOT_UNIQUE,
				  "(;FF[4];AB[aa][aa];MA[kk]TR[kk])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];AB[aa];MA[kk])\n");
}
END_TEST


START_TEST (test_WS_ADDSTONE_REDUNDANT)
{
	trigger_error(WS_ADDSTONE_REDUNDANT,
				  "(;B[cc];AB[cc])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[cc];)\n");
}
END_TEST


START_TEST (test_WS_PROPERTY_NOT_IN_FF)
{
	trigger_error(WS_PROPERTY_NOT_IN_FF,
				  "(;FF[4];L[aa][bb][cc])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];LB[aa:a][bb:b][cc:c])\n");
}
END_TEST


START_TEST (test_E_ANNOTATE_NOT_UNIQUE)
{
	trigger_error(E_ANNOTATE_NOT_UNIQUE,
				  "(;FF[4];GB[2]GW[1])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];GB[2])\n");
}
END_TEST


START_TEST (test_E4_BM_TE_IN_NODE)
{
	trigger_error(E4_BM_TE_IN_NODE,
				  "(;B[cc]TE[1]BM[1];W[dd]BM[1]TE[1])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]B[cc]IT[];W[dd]DO[])\n");
}
END_TEST


START_TEST (test_E_ANNOTATE_WITHOUT_MOVE)
{
	trigger_error(E_ANNOTATE_WITHOUT_MOVE,
				  "(;TE[2])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_E4_GINFO_ALREADY_SET)
{
	trigger_error(E4_GINFO_ALREADY_SET,
				  "(;GN[test];HA[4])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nGN[test]\n\n;)\n");
}
END_TEST


START_TEST (test_WS_FF_DIFFERS)
{
	trigger_error(WS_FF_DIFFERS,
				  "(;GN[1]) (;FF[3]GN[2])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nGN[1]\n\n)\n"
	  			  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nGN[2]\n\n)\n");
}
END_TEST


START_TEST (test_E_SQUARE_AS_RECTANGULAR)
{
	trigger_error(E_SQUARE_AS_RECTANGULAR,
				  "(;FF[4]SZ[13:13])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[13])\n");
}
END_TEST


START_TEST (test_E_BOARD_TOO_BIG)
{
	trigger_error(E_BOARD_TOO_BIG,
				  "(;FF[4]SZ[1000]) (;FF[4]SZ[10:53])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[52])\n(;FF[4]CA[UTF-8]GM[1]SZ[10:52])\n");
}
END_TEST


START_TEST (test_E_VERSION_CONFLICT)
{
	trigger_error(E_VERSION_CONFLICT,
				  "(;FF[3]SZ[13:9];AB[aa:ee])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[13:9];AB[aa:ee])\n");
}
END_TEST


START_TEST (test_E_BAD_VW_VALUES)
{
	trigger_error(E_BAD_VW_VALUES,
				  "(;FF[3]VW[aj][ak][al][am])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]VW[aj:am])\n");
}
END_TEST


START_TEST (test_WS_GM_DIFFERS)
{
{
	allowed_error = WCS_GAME_NOT_GO;
	trigger_error(WS_GM_DIFFERS,
				  "(;GM[1])(;GM[2])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n(;FF[4]CA[UTF-8]GM[2])\n");
}
}
END_TEST


START_TEST (test_E_VALUES_WITHOUT_ID)
{
	trigger_error(E_VALUES_WITHOUT_ID,
				  "(;[ab][ac])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_W_EMPTY_NODE_DELETED)
{
	sgfc->options->del_empty_nodes = true;
	trigger_error(W_EMPTY_NODE_DELETED,
				  "(;;;C[empty])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]C[empty])\n");
}
END_TEST


START_TEST (test_W_VARLEVEL_UNCERTAIN)
{
	sgfc->options->fix_variation = true;
	trigger_error(W_VARLEVEL_UNCERTAIN,
				  "(;;B[dd];W[aa](;B[bb])(;AE[aa];W[ba])(;AE[dd][aa];B[ef]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[dd];W[aa]\n"
	  			  "(;B[bb])\n(;AE[aa];W[ba])\n(;AE[aa][dd];B[ef]))\n");
}
END_TEST


START_TEST (test_W_VARLEVEL_CORRECTED)
{
	sgfc->options->fix_variation = true;
	trigger_error(W_VARLEVEL_CORRECTED,
				  "(;GM[1];W[aa](;B[bb])(;AE[aa];W[ba])(;AE[aa];W[ef]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n(;W[aa];B[bb])\n(;W[ba])\n(;W[ef]))\n");
}
END_TEST


START_TEST (test_WS_ILLEGAL_MOVE)
{
	trigger_error(WS_ILLEGAL_MOVE,
				  "(;GM[1];B[aa];W[aa])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[aa];W[aa])\n");
}
END_TEST


START_TEST (test_W_INT_KOMI_FOUND)
{
	trigger_error(W_INT_KOMI_FOUND,
				  "(;KI[11])(;KM[3.5]KI[8])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nKM[5.5]\n\n)\n"
	  			  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nKM[3.5]\n\n)\n");
}
END_TEST


START_TEST (test_E_MORE_THAN_ONE_TREE)
{
	sgfc->options->strict_checking = true;
	trigger_error(E_MORE_THAN_ONE_TREE,
				  "(;GM[1])(;GM[1])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n"
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19])\n");
}
END_TEST


START_TEST (test_W_HANDICAP_NOT_SETUP)
{
	sgfc->options->strict_checking = true;
	allowed_error = E_MORE_THAN_ONE_TREE;
	trigger_error(W_HANDICAP_NOT_SETUP,
				  "(;GM[1]AB[aa][bb])(;GM[1]HA[3];B[bb])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]AB[aa][bb])\n"
	  			  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n\nHA[3]\n\n;B[bb])\n");
}
END_TEST


START_TEST (test_W_SETUP_AFTER_ROOT)
{
	sgfc->options->strict_checking = true;
	trigger_error(W_SETUP_AFTER_ROOT,
				  "(;GM[1];W[cc];AB[aa]AE[cc])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];W[cc];AB[aa]AE[cc])\n");
}
END_TEST


START_TEST (test_W_MOVE_OUT_OF_SEQUENCE)
{
	sgfc->options->strict_checking = true;
	trigger_error(W_MOVE_OUT_OF_SEQUENCE,
				  "(;GM[1];B[dd];W[cc];W[ee])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[dd];W[cc];W[ee])\n");
}
END_TEST


START_TEST (test_E_FF4_PASS_IN_OLD_FF)
{
	trigger_error(E_FF4_PASS_IN_OLD_FF,
				  "(;GM[1]FF[3];B[])",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19];B[])\n");
}
END_TEST


START_TEST (test_E_NODE_OUTSIDE_VAR)
{
	trigger_error(E_NODE_OUTSIDE_VAR,
				  "(;FF[4](;C[var 1]);C[var 2]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n(;C[var 1])\n(;C[var 2]))\n");
}
END_TEST


START_TEST (test_E_MISSING_NODE_START)
{
	trigger_error(E_MISSING_NODE_START,
				  "(;FF[4](;C[var 1]) ( C[var 2]))",
				  "(;FF[4]CA[UTF-8]GM[1]SZ[19]\n(;C[var 1])\n(;C[var 2]))\n");
}
END_TEST


TCase *sgfc_tc_trigger_errors(void)
{
	TCase *tc;

	tc = tcase_create("trigger_errors");
	tcase_add_checked_fixture(tc, setup, common_teardown);

	/* errors 1..5 missing */
	tcase_add_test(tc, test_W_SGF_IN_HEADER);
	tcase_add_test(tc, test_FE_NO_SGFDATA);
	tcase_add_test(tc, test_E_ILLEGAL_OUTSIDE_CHARS);
	tcase_add_test(tc, test_E_VARIATION_NESTING);
	tcase_add_test(tc, test_E_UNEXPECTED_EOF);

	tcase_add_test(tc, test_E_PROPID_TOO_LONG);
	tcase_add_test(tc, test_E_EMPTY_VARIATION);
	tcase_add_test(tc, test_E_TOO_MANY_VALUES);
	tcase_add_test(tc, test_E_BAD_VALUE_DELETED);
	tcase_add_test(tc, test_E_BAD_VALUE_CORRECTED);
	tcase_add_test(tc, test_E_LC_IN_PROPID);
	tcase_add_test(tc, test_E_EMPTY_VALUE_DELETED);
	tcase_add_test(tc, test_W_EMPTY_VALUE_DELETED);
	tcase_add_test(tc, test_E_BAD_ROOT_PROP);
	tcase_add_test(tc, test_WCS_GAME_NOT_GO);
	tcase_add_test(tc, test_E_NO_PROP_VALUES);

	tcase_add_test(tc, test_E_VARIATION_START);
	/* error 22 missing */
	tcase_add_test(tc, test_E_COMPOSE_EXPECTED);
	tcase_add_test(tc, test_WS_MOVE_IN_ROOT);
	tcase_add_test(tc, test_E_BAD_COMPOSE_CORRECTED);
	/* errors 26+27 missing */
	tcase_add_test(tc, test_E_DOUBLE_PROP);
	tcase_add_test(tc, test_W_PROPERTY_DELETED);
	tcase_add_test(tc, test_E4_MOVE_SETUP_MIXED);

	tcase_add_test(tc, test_WS_LONG_PROPID);
	tcase_add_test(tc, test_E_ROOTP_NOT_IN_ROOTN);
	tcase_add_test(tc, test_E4_FAULTY_GC);
	/* error 34 missing */
	tcase_add_test(tc, test_WS_UNKNOWN_PROPERTY);
	tcase_add_test(tc, test_E_MISSING_SEMICOLON);
	tcase_add_test(tc, test_E_TWO_MOVES_IN_NODE);
	tcase_add_test(tc, test_E_POSITION_NOT_UNIQUE);
	tcase_add_test(tc, test_WS_ADDSTONE_REDUNDANT);
	tcase_add_test(tc, test_WS_PROPERTY_NOT_IN_FF);

	tcase_add_test(tc, test_E_ANNOTATE_NOT_UNIQUE);
	tcase_add_test(tc, test_E4_BM_TE_IN_NODE);
	tcase_add_test(tc, test_E_ANNOTATE_WITHOUT_MOVE);
	tcase_add_test(tc, test_E4_GINFO_ALREADY_SET);
	tcase_add_test(tc, test_WS_FF_DIFFERS);
	/* error 46 missing */
	tcase_add_test(tc, test_E_SQUARE_AS_RECTANGULAR);
	/* errors 48+49 missing */
	tcase_add_test(tc, test_E_BOARD_TOO_BIG);

	tcase_add_test(tc, test_E_VERSION_CONFLICT);
	tcase_add_test(tc, test_E_BAD_VW_VALUES);
	tcase_add_test(tc, test_WS_GM_DIFFERS);
	tcase_add_test(tc, test_E_VALUES_WITHOUT_ID);
	tcase_add_test(tc, test_W_EMPTY_NODE_DELETED);
	tcase_add_test(tc, test_W_VARLEVEL_UNCERTAIN);
	tcase_add_test(tc, test_W_VARLEVEL_CORRECTED);
	tcase_add_test(tc, test_WS_ILLEGAL_MOVE);
	tcase_add_test(tc, test_W_INT_KOMI_FOUND);
	tcase_add_test(tc, test_E_MORE_THAN_ONE_TREE);

	tcase_add_test(tc, test_W_HANDICAP_NOT_SETUP);
	tcase_add_test(tc, test_W_SETUP_AFTER_ROOT);
	tcase_add_test(tc, test_W_MOVE_OUT_OF_SEQUENCE);
	/* error 64 missing */
	tcase_add_test(tc, test_E_FF4_PASS_IN_OLD_FF);
	tcase_add_test(tc, test_E_NODE_OUTSIDE_VAR);
	tcase_add_test(tc, test_E_MISSING_NODE_START);
	/* error 68 missing */

	return tc;
}
