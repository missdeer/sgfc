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
*** Function:	Crosscheck_Handicap
***				Check if HA[] value matches number of setup stones (AB[])
*** Parameters: root ... root node of gametree
*** Returns:	-
**************************************************************************/

static void Crosscheck_Handicap(struct SGFInfo *sgfc, struct Node *root)
{
	int setupstones = 0;
	struct Property *prop;
	char *buff = NULL;
	
	if((prop = Find_Property(root, TKN_AB))) /* handicap means black stones */
	{
		buff = prop->buffer;
		/* if there's an AB but no AW than it is likely to be
		 * a handicap game, otherwise it's a position setup */
		if(!Find_Property(root, TKN_AW))
		{
			struct PropValue *val = prop->value;
			/* count number of handicap stones */
			while (val)
			{
				setupstones++;
				val = val->next;
			}
		}
	}

	if((prop = Find_Property(root, TKN_HA))) /* handicap game info */
	{
		if(atoi(prop->value->value) != setupstones)
			PrintError(W_HANDICAP_NOT_SETUP, sgfc, prop->buffer);
	}
	else if(setupstones != 0)
		PrintError(W_HANDICAP_NOT_SETUP, sgfc, buff);
}


/**************************************************************************
*** Function:	Check_Move_Order
***				Check that there are no two successive moves of the
***				same color; check that there are no setup stones (AB/AW/AE)
***				outside the root node
*** Parameters: node ... root's first child node
*** Returns:	-
**************************************************************************/

static void Check_Move_Order(struct SGFInfo *sgfc, struct Node *node, int checkSetup)
{
	int old_col = 0;

	if(!node)		/* does tree only consist of root node? */
		return;
	while(node)
	{
		if(Find_Property(node, TKN_AB) || Find_Property(node, TKN_AW)
		|| Find_Property(node, TKN_AE))
		{
			if(checkSetup)
				PrintError(W_SETUP_AFTER_ROOT, sgfc, node->buffer);
			else
				old_col = 0;
		}

		if(Find_Property(node, TKN_B))
		{
			if(old_col && old_col != TKN_W)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->buffer);
			old_col = TKN_B;
		}
		if(Find_Property(node, TKN_W))
		{
			if(old_col && old_col != TKN_B)
				PrintError(W_MOVE_OUT_OF_SEQUENCE, sgfc, node->buffer);
			old_col = TKN_W;
		}
		if (node->sibling)
			Check_Move_Order(sgfc, node->sibling, FALSE);
		node = node->child;
	}
}


/**************************************************************************
*** Function:	Strict_Checking
***				Perform various checks on the SGF file
*** Parameters: sgfc ... SGFInfo structure
*** Returns:	-
**************************************************************************/

void Strict_Checking(struct SGFInfo *sgfc)
{
	struct TreeInfo *tree;

	if(sgfc->tree != sgfc->last)
		PrintError(E_MORE_THAN_ONE_TREE, sgfc);

	tree = sgfc->tree;
	while(tree)
	{
		if(tree->GM == 1)
		{
			Crosscheck_Handicap(sgfc, tree->root);
			Check_Move_Order(sgfc, tree->root->child, TRUE);
		}
		tree = tree->next;
	}

	/* TODO: delete pass moves at end (?) */
}
