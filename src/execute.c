/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 execute.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	MakeCapture
***				Captures stones on marked by RecursiveCapture
*** Parameters: x ... x position
***				y ... y position
*** Returns:	-
**************************************************************************/

static void MakeCapture(int x, int y, struct BoardStatus *st)
{
	if(st->paths->board[MXY(x,y)] != st->paths->num)
		return;

	st->board[MXY(x,y)] = EMPTY;
	st->paths->board[MXY(x,y)] = 0;

	/* recursive calls */
	if(x > 0) MakeCapture(x - 1, y, st);
	if(y > 0) MakeCapture(x, y - 1, st);
	if(x < st->bwidth-1) MakeCapture(x + 1, y, st);
	if(y < st->bheight-1) MakeCapture(x, y + 1, st);
}


/**************************************************************************
*** Function:	RecursiveCapture
***				Checks for capturing stones
***				marks its path on path_board with path_num
*** Parameters: color ... enemy color
***				x	  ... x position
***				y	  ... y position
*** Returns:	true if capture / false if liberty found
**************************************************************************/

static bool RecursiveCapture(unsigned char color, int x, int y, struct BoardStatus *st)
{
	if(!st->board[MXY(x,y)])
		return false;		/* liberty found */

	if(st->board[MXY(x,y)] == color)
		return true;		/* enemy found */

	if(st->paths->board[MXY(x,y)] == st->paths->num)
		return true;

	st->paths->board[MXY(x,y)] = st->paths->num;

	/* recursive calls */
	if(x > 0 			 && !RecursiveCapture(color, x - 1, y, st))	return false;
	if(y > 0 			 && !RecursiveCapture(color, x, y - 1, st))	return false;
	if(x < st->bwidth-1  && !RecursiveCapture(color, x + 1, y, st))	return false;
	if(y < st->bheight-1 && !RecursiveCapture(color, x, y + 1, st))	return false;

	return true;
}


/**************************************************************************
*** Function:	CaptureStones
***				Calls RecursiveCapture and does capture if necessary
*** Parameters: st	  ... board status
***				color ... color of enemy stones
***				x	  ... column
***				y	  ... row
*** Returns:	-
**************************************************************************/

static void CaptureStones(struct BoardStatus *st, unsigned char color, int x, int y)
{
	if(x < 0 || y < 0 || x >= st->bwidth || y >= st->bheight)
		return;		/* not on board */

	if(!st->board[MXY(x,y)] || st->board[MXY(x,y)] == color)
		return;		/* liberty or friend found */

	st->paths->num++;

	if(RecursiveCapture(color, x, y, st))	/* made prisoners? */
		MakeCapture(x, y, st);				/* ->update board position */
}



/**************************************************************************
*** Function:	Do_Move
***				Executes a move and check for B/W in one node
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Move(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	int x, y;
	unsigned char color;

	if(sgfc->info->GM != 1)		/* game != Go? */
		return true;

	if(st->annotate & ST_MOVE)	/* there's a move already? */
	{
		PrintError(E_TWO_MOVES_IN_NODE, sgfc, p->row, p->col);
		SplitNode(sgfc, n, 0, p->id, true);
		return true;
	}

	st->annotate |= ST_MOVE;

	if(!p->value->value_len)	/* pass move */
		return true;

	x = DecodePosChar(p->value->value[0]) - 1;
	y = DecodePosChar(p->value->value[1]) - 1;
	color = (unsigned char)sgf_token[p->id].data;

	if(st->board[MXY(x,y)])
		PrintError(WS_ILLEGAL_MOVE, sgfc, p->row, p->col);

	st->board[MXY(x,y)] = color;
	CaptureStones(st, color, x - 1, y);		/* check for prisoners */
	CaptureStones(st, color, x + 1, y);
	CaptureStones(st, color, x, y - 1);
	CaptureStones(st, color, x, y + 1);
	CaptureStones(st, (unsigned char)~color, x, y);		/* check for suicide */

	if(sgfc->options->del_move_markup)		/* if del move markup, then */
	{										/* mark move position as markup */
		st->markup[MXY(x,y)] |= ST_MARKUP;	/* -> other markup at this */
		st->markup_changed = true;			/* position will be deleted */
	}

	return true;
}


/**************************************************************************
*** Function:	Do_AddStones
***				Executes property checks for unique positions
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_AddStones(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	int x, y;
	unsigned char color;
	struct PropValue *v;

	if(sgfc->info->GM != 1)		/* game != Go? */
		return true;

	color = (unsigned char)sgf_token[p->id].data;

	v = p->value;
	while(v)
	{
		x = DecodePosChar(v->value[0]) - 1;
		y = DecodePosChar(v->value[1]) - 1;

		if(st->markup[MXY(x,y)] & ST_ADDSTONE)
		{
			PrintError(E_POSITION_NOT_UNIQUE, sgfc, v->row, v->col, v->value, "AddStone", p->idstr);
			v = DelPropValue(p, v);
			continue;
		}

		st->markup[MXY(x,y)] |= ST_ADDSTONE;
		st->markup_changed = true;

		if(st->board[MXY(x,y)] == color)		/* Add property is redundant */
		{
			PrintError(WS_ADDSTONE_REDUNDANT, sgfc, v->row, v->col, v->value, p->idstr);
			v = DelPropValue(p, v);
			continue;
		}

		st->board[MXY(x,y)] = color;
		v = v->next;
	}

	return true;
}


/**************************************************************************
*** Function:	Do_Letter
***				Converts L to LB values / checks unique position
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Letter(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	int x, y;
	struct PropValue *v;
	char letter[2] = "a";

	if(sgfc->info->GM != 1)		/* game != Go? */
		return true;

	v = p->value;
	while(v)
	{
		x = DecodePosChar(v->value[0]) - 1;
		y = DecodePosChar(v->value[1]) - 1;

		if(st->markup[MXY(x,y)] & ST_LABEL)
		{
			PrintError(E_POSITION_NOT_UNIQUE, sgfc, v->row, v->col, v->value, "Label", p->idstr);
		}
		else
		{
			st->markup[MXY(x,y)] |= ST_LABEL;
			st->markup_changed = true;
			NewPropValue(sgfc, n, TKN_LB, v->value, letter, false);
			letter[0]++;
		}

		v = v->next;
	}

	return false;
}


/**************************************************************************
*** Function:	Do_Mark
***				Converts M to MA/TR depending on board / checks uniqueness
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Mark(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	int x, y;
	struct PropValue *v;

	if(sgfc->info->GM != 1)		/* game != Go? */
		return true;

	v = p->value;
	while(v)
	{
		x = DecodePosChar(v->value[0]) - 1;
		y = DecodePosChar(v->value[1]) - 1;

		if(st->markup[MXY(x,y)] & ST_MARKUP)
		{
			PrintError(E_POSITION_NOT_UNIQUE, sgfc, v->row, v->col, v->value, "Markup", p->idstr);
		}
		else
		{
			st->markup[MXY(x,y)] |= ST_MARKUP;
			st->markup_changed = true;

			if(st->board[MXY(x,y)])
				NewPropValue(sgfc, n, TKN_TR, v->value, NULL, false);
			else
				NewPropValue(sgfc, n, TKN_MA, v->value, NULL, false);
		}
		v = v->next;
	}

	return false;
}


/**************************************************************************
*** Function:	Do_Markup
***				Checks unique positions for markup properties
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Markup(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	int x, y;
	struct PropValue *v;
	U_SHORT flag;
	bool empty, not_empty;

	if(sgfc->info->GM != 1)		/* game != Go? */
		return true;

	v = p->value;
	flag = sgf_token[p->id].data;
	empty = false;
	not_empty = false;

	while(v)
	{
		if(!v->value_len)
		{
			if(empty)	/* if we already have an empty value */
			{
				PrintError(E_EMPTY_VALUE_DELETED, sgfc, v->row, v->col, "Markup", p->idstr);
				v = DelPropValue(p, v);
				continue;
			}
			empty = true;
			v = v->next;
			continue;
		}

		not_empty = true;
		x = DecodePosChar(v->value[0]) - 1;
		y = DecodePosChar(v->value[1]) - 1;

		if(st->markup[MXY(x,y)] & flag)
		{
			PrintError(E_POSITION_NOT_UNIQUE, sgfc, v->row, v->col, v->value, "Markup", p->idstr);
			v = DelPropValue(p, v);
			continue;
		}

		st->markup[MXY(x,y)] |= flag;
		st->markup_changed = true;
		v = v->next;
	}

	if(empty && not_empty)	/* if we have both empty and non-empty values: delete empty values */
	{
		v = p->value;
		while(v) {
			if(!v->value_len)
			{
				PrintError(E_EMPTY_VALUE_DELETED, sgfc, v->row, v->col, "Markup", p->idstr);
				v = DelPropValue(p, v);
				continue;
			}
			v = v->next;
		}
	}

	return true;
}


/**************************************************************************
*** Function:	Do_Annotate
***				Checks annotation properties / converts BM_TE / TE_BM
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Annotate(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	struct Property *hlp;
	U_SHORT flag;

	flag = sgf_token[p->id].data;

	if((st->annotate & ST_ANN_BM) && p->id == TKN_TE) /* DO (doubtful) */
	{
		PrintError(E4_BM_TE_IN_NODE, sgfc, p->row, p->col, "BM-TE", "DO");
		hlp = FindProperty(n, TKN_BM);
		hlp->id = TKN_DO;
		free(hlp->idstr);
		hlp->idstr = SaveDupString(sgf_token[TKN_DO].id, 0, "DO id string");
		hlp->value->value[0] = 0;
		hlp->value->value_len = 0;
		return false;
	}

	if(st->annotate & ST_ANN_TE && p->id == TKN_BM)	/* IT (interesting) */
	{
		PrintError(E4_BM_TE_IN_NODE, sgfc, p->row, p->col, "TE-BM", "IT");
		hlp = FindProperty(n, TKN_TE);
		hlp->id = TKN_IT;
		free(hlp->idstr);
		hlp->idstr = SaveDupString(sgf_token[TKN_IT].id, 0, "DO id string");
		hlp->value->value[0] = 0;
		hlp->value->value_len = 0;
		return false;
	}

	if(st->annotate & flag)
	{
		PrintError(E_ANNOTATE_NOT_UNIQUE, sgfc, p->row, p->col, p->idstr);
		return false;
	}

	if((flag & (ST_ANN_MOVE|ST_KO)) && !(st->annotate & ST_MOVE))
	{
		PrintError(E_ANNOTATE_WITHOUT_MOVE, sgfc, p->row, p->col, p->idstr);
		return false;
	}

	st->annotate |= flag;
	return true;
}


/**************************************************************************
*** Function:	Do_Root
***				Checks if root properties are stored in root
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_Root(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	if(n->parent)
	{
		PrintError(E_ROOTP_NOT_IN_ROOTN, sgfc, p->row, p->col, p->idstr);
		return false;
	}
	return true;
}


/**************************************************************************
*** Function:	Do_GInfo
***				checks for uniqueness of properties within the tree
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_GInfo(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	long ki;
	char *new_km;

	if(st->ginfo && (st->ginfo != n))
	{
		PrintError(E4_GINFO_ALREADY_SET, sgfc, p->row, p->col, p->idstr,
			 	   st->ginfo->row, st->ginfo->col);
		return false;
	}

	st->ginfo = n;
	if(p->id != TKN_KI)
		return true;

	if(FindProperty(n, TKN_KM))
		PrintError(W_INT_KOMI_FOUND, sgfc, p->row, p->col, "deleted (<KM> property found)");
	else
	{
		PrintError(W_INT_KOMI_FOUND, sgfc, p->row, p->col, "converted to <KM>");

		ki = strtol(p->value->value, NULL, 10);		/* we can ignore errors here */
		new_km = SaveMalloc(p->value->value_len+3, "new KM number value");
		if(ki % 2)	sprintf(new_km, "%ld.5", ki/2);
		else		sprintf(new_km, "%ld", ki/2);
		NewPropValue(sgfc, n, TKN_KM, new_km, NULL, false);
		free(new_km);
	}
	return false;
}


/**************************************************************************
*** Function:	Do_View
***				checks and converts VW property
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... Node that contains the property
***				p	 ... property
***				st	 ... current board status
*** Returns:	true: ok / false: delete property
**************************************************************************/

bool Do_View(struct SGFInfo *sgfc, struct Node *n, struct Property *p, struct BoardStatus *st)
{
	struct PropValue *v;
	int i = 0;

	v = p->value;

	if(!v->value_len)	/* VW[] */
	{
		if(v->next)
		{
			PrintError(E_BAD_VW_VALUES, sgfc, p->row, p->col,
			  		   "values after '[]' value found", "deleted");
			v = v->next;
			while(v)
				v = DelPropValue(p, v);
		}

		return true;
	}

	while(v)
	{
		if(!v->value_len)	/* '[]' within other values */
		{
			PrintError(E_BAD_VW_VALUES, sgfc, v->row, v->col,
			  		   "empty value found in list", "deleted");
			v = DelPropValue(p, v);
		}
		else
		{
			i++;
			v = v->next;
		}
	}

	if(sgfc->info->GM != 1)		/* game not Go */
		return true;

	if(sgfc->info->FF < 4)		/* old definition of VW */
	{
		if(i == 2)				/* transform FF3 values */
		{
			v = p->value;
			v->value2 = v->next->value;
			v->next->value = NULL;
			DelPropValue(p, v->next);

			if(!ExpandPointList(sgfc, p, v, false))
			{
				PrintError(E_BAD_VW_VALUES, sgfc, v->row, v->col,
			   			   "illegal FF[3] definition", "deleted");
				return false;
			}

			DelPropValue(p, v);
		}
		else		/* looks like FF4 definition (wrong FF set?) */
			PrintError(E_BAD_VW_VALUES, sgfc, p->row, p->col,
			  		   "FF[4] definition in older FF found", "parsing done anyway");
	}

	return true;
}
