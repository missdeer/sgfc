/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 strict.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	CheckHandicap
***				Check if HA[] value matches number of setup stones (AB[])
*** Parameters: sgfc ... pointer to SGFInfo structure
***				root ... root node of gametree
*** Returns:	-
**************************************************************************/

static void CheckHandicap(struct SGFInfo *sgfc, struct Node *root)
{
	int setup_stones = 0;
	struct Property *handicap, *addBlack;

	if((addBlack = FindProperty(root, TKN_AB))) /* handicap means black stones */
	{
		/* if there's an AB but no AW than it is likely to be
		 * a handicap game, otherwise it's a position setup */
		if(!FindProperty(root, TKN_AW))
		{
			struct PropValue *val = addBlack->value;
			/* count number of handicap stones */
			while (val)
			{
				setup_stones++;
				val = val->next;
			}
		}
	}

	if((handicap = FindProperty(root, TKN_HA))) /* handicap game info */
	{
		if(atoi(handicap->value->value) != setup_stones)
			PrintError(W_HANDICAP_NOT_SETUP, sgfc, handicap->row, handicap->col);
	}
	else if(setup_stones != 0)
		PrintError(W_HANDICAP_NOT_SETUP, sgfc, addBlack->row, addBlack->col);
}


/**************************************************************************
*** Function:	CheckMoveOrder
***				Check that there are no two successive moves of the
***				same color; check that there are no setup stones (AB/AW/AE)
***				outside the root node
*** Parameters: sgfc ... pointer to SGFInfo structure
***				node ... root's first child node
***				check_setup ... whether setup stones outside root node should be checked
*** Returns:	-
**************************************************************************/

static void CheckMoveOrder(struct SGFInfo *sgfc, struct Node *node, bool check_setup)
{
	int old_col = 0;

	if(!node)		/* does tree only consist of root node? */
		return;
	while(node)
	{
		if(FindProperty(node, TKN_AB) || FindProperty(node, TKN_AW)
		   || FindProperty(node, TKN_AE))
		{
			if(check_setup)
				PrintError(W_SETUP_AFTER_ROOT, sgfc, node->row, node->col);
			else
				old_col = 0;
		}

		if(FindProperty(node, TKN_B))
		{
			if(old_col && old_col != TKN_W)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->row, node->col);
			old_col = TKN_B;
		}
		if(FindProperty(node, TKN_W))
		{
			if(old_col && old_col != TKN_B)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->row, node->col);
			old_col = TKN_W;
		}
		if (node->sibling)
			CheckMoveOrder(sgfc, node->sibling, false);
		node = node->child;
	}
}


/**************************************************************************
*** Function:	StrictChecking
***				Perform various checks on the SGF file
*** Parameters: sgfc ... SGFInfo structure
*** Returns:	-
**************************************************************************/

void StrictChecking(struct SGFInfo *sgfc)
{
	struct TreeInfo *tree;

	if(sgfc->tree != sgfc->last)
		PrintError(E_MORE_THAN_ONE_TREE, sgfc);

	tree = sgfc->tree;
	while(tree)
	{
		if(tree->GM == 1)
		{
			CheckHandicap(sgfc, tree->root);
			CheckMoveOrder(sgfc, tree->root->child, true);
		}
		tree = tree->next;
	}

	/* TODO: delete pass moves at end (?) */
}
