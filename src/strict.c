/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 strict.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	struct Property *prop;
	char *buff = NULL;
	
	if((prop = FindProperty(root, TKN_AB))) /* handicap means black stones */
	{
		buff = prop->buffer;
		/* if there's an AB but no AW than it is likely to be
		 * a handicap game, otherwise it's a position setup */
		if(!FindProperty(root, TKN_AW))
		{
			struct PropValue *val = prop->value;
			/* count number of handicap stones */
			while (val)
			{
				setup_stones++;
				val = val->next;
			}
		}
	}

	if((prop = FindProperty(root, TKN_HA))) /* handicap game info */
	{
		if(atoi(prop->value->value) != setup_stones)
			PrintError(W_HANDICAP_NOT_SETUP, sgfc, prop->buffer);
	}
	else if(setup_stones != 0)
		PrintError(W_HANDICAP_NOT_SETUP, sgfc, buff);
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

static void CheckMoveOrder(struct SGFInfo *sgfc, struct Node *node, int check_setup)
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
				PrintError(W_SETUP_AFTER_ROOT, sgfc, node->buffer);
			else
				old_col = 0;
		}

		if(FindProperty(node, TKN_B))
		{
			if(old_col && old_col != TKN_W)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->buffer);
			old_col = TKN_B;
		}
		if(FindProperty(node, TKN_W))
		{
			if(old_col && old_col != TKN_B)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->buffer);
			old_col = TKN_W;
		}
		if (node->sibling)
			CheckMoveOrder(sgfc, node->sibling, FALSE);
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
			CheckMoveOrder(sgfc, tree->root->child, TRUE);
		}
		tree = tree->next;
	}

	/* TODO: delete pass moves at end (?) */
}
