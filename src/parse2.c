/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 parse.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	ExpandPointList
***				Expanding compressed pointlists
*** Parameters: sgfc  ... pointer to SGFInfo structure
***				p	  ... property
***				v	  ... compose value to be expanded
***				print_error ... if true print errors
*** Returns:	true if success / false on error (exits on low memory)
**************************************************************************/

bool ExpandPointList(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v, bool print_error)
{
	char val[2];
	int h = 0;

	int x1 = DecodePosChar(v->value[0]);
	int y1 = DecodePosChar(v->value[1]);
	int x2 = DecodePosChar(v->value2[0]);
	int y2 = DecodePosChar(v->value2[1]);

	if(x1 == x2 && y1 == y2)	/* illegal definition */
	{
		free(v->value2);
		v->value2 = NULL;
		if(print_error)
			PrintError(E_BAD_VALUE_CORRECTED, sgfc, v->row, v->col, v->value, p->idstr, v->value);
		return false;
	}

	if(x1 > x2)					/* encoded as [ul:lr] ? */
	{
		h = x1; x1 = x2; x2 = h;
		v->value[0] = EncodePosChar(x1);
		v->value2[0] = EncodePosChar(x2);
	}

	if(y1 > y2)
	{
		h = y1; y1 = y2; y2 = h;
		v->value[1] = EncodePosChar(y1);
		v->value2[1] = EncodePosChar(y2);
	}

	if(h && print_error)
		PrintError(E_BAD_COMPOSE_CORRECTED, sgfc, v->row, v->col, v->value, p->idstr, v->value, v->value2);

	for(; x1 <= x2; x1++)		/* expand point list */
		for(h = y1; h <= y2; h++)
		{
			val[0] = EncodePosChar(x1);
			val[1] = EncodePosChar(h);
			AddPropValue(sgfc, p, v->row, v->col, val, 2, NULL, 0);
		}

	return true;
}


/**************************************************************************
*** Function:	CompressPointList
***				A simple greedy algorithm to compress pointlists
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... property which list should get compressed
*** Returns:	- (exits on low memory)
**************************************************************************/

void CompressPointList(struct SGFInfo *sgfc, struct Property *p)
{
	char board[MAX_BOARDSIZE+2][MAX_BOARDSIZE+2];
	int x, y, yy, i, j, m, mx, my;
	bool expx, expy;
	struct PropValue *v;
	char val1[12], val2[2];

	memset(board, 0, (MAX_BOARDSIZE+2)*(MAX_BOARDSIZE+2));

	x = yy = MAX_BOARDSIZE+10;
	mx = my = 0;

	v = p->value;
	while(v)		/* generate board position & delete old values */
	{
		if(strlen(v->value))
		{
			i = DecodePosChar(v->value[0]);
			j = DecodePosChar(v->value[1]);
			board[i][j] = 1;
			if(x > i)	x = i;						/* get minimum */
			if(yy > j)	yy = j;
			if(mx < i)	mx = i;						/* get maximum */
			if(my < j)	my = j;

			v = DelPropValue(p, v);
		}
		else
			v = v->next;
	}

	for(; x <= mx; x++)							/* search whole board */
		for(y = yy; y <= my; y++)
			if(board[x][y])						/* starting point found */
			{									/* --> ul corner */
				i = x;	j = y;
				expx = true;	expy = true;
				while(expx || expy)				/* still enlarging area? */
				{
					if(board[i+1][y] && expx)
					{
						for(m = y; m <= j; m++)
							if(!board[i+1][m])
								break;
						if(m > j)				/* new column ok? */
							i++;
						else
							expx = false;		/* x limit reached */
					}
					else
						expx = false;

					if(board[x][j+1] && expy)
					{
						for(m = x; m <= i; m++)
							if(!board[m][j+1])
								break;
						if(m > i)				/* new row ok? */
							j++;
						else
							expy = false;		/* y limit reached */
					}
					else
						expy = false;
				}

				val1[0] = EncodePosChar(x);
				val1[1] = EncodePosChar(y);
				val2[0] = EncodePosChar(i);
				val2[1] = EncodePosChar(j);

				if(x != i || y != j)			/* Add new values to property */
					AddPropValue(sgfc, p, 0, 0, val1, 2, val2, 2);
				else
					AddPropValue(sgfc, p, 0, 0, val1, 2, NULL, 0);

				for(; i >= x; i--)				/* remove points from board */
					for(m = j; m >= y; m--)
						board[i][m] = 0;
			}
}


/**************************************************************************
*** Function:	CorrectVariation
***				Checks if variation criteria fit and corrects level
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... first node of siblings (child of parent)
*** Returns:	-
**************************************************************************/

static void CorrectVariation(struct SGFInfo *sgfc, struct Node *n)
{
	struct Node *p, *i, *j, *k;
	struct Property *pmv, *w, *b, *ae;
	int fault = 0, success = 0;

	p = n->parent;
	if(!p)					/* don't mess with root nodes */
		return;

	if(p->parent)			/* Does parent have siblings? */
		if(p->sibling || (p->parent->child != p))
			return;

	pmv = FindProperty(p, TKN_B);
	if(!pmv)
	{
		pmv = FindProperty(p, TKN_W);
		if(!pmv)
			return;			/* no move in parent found */
	}

	i = n->sibling;
	while(i)					/* check variations */
	{
		fault++;
		j = i;
		i = i->sibling;

		ae = FindProperty(j, TKN_AE);
		if(ae)
		{
			if(ae->value->next)			/* AE has more than one value */
				continue;
			if(strcmp(ae->value->value, pmv->value->value))
				continue;				/* AE doesn't undo parent move */

			w = FindProperty(j, TKN_AW);
			b = FindProperty(j, TKN_AB);

			if(w || b)					/* AB or AW in same node as AE */
			{
				if(w && b)				/* AW and AB in same node */
					continue;

				if(w)	b = w;
				if(b->value->next)		/* AB/AW has more than one value */
					continue;

				if(b->id == TKN_AW)		b->id = TKN_W;
				else					b->id = TKN_B;
				b->flags = sgf_token[b->id].flags;	/* update local copy */

				SplitNode(sgfc, j, TYPE_SETUP | TYPE_ROOT | TYPE_GINFO, TKN_N, false);
			}

			if(j->child)				/* variation must have a child */
			{
				if(j->child->sibling)	/* but child must not have siblings */
					continue;

				if(FindProperty(j->child, TKN_W) || FindProperty(j->child, TKN_B))
				{
					w = j->prop;		/* check for properties which occur */
					while(w)			/* in node AND child */
					{
						if(FindProperty(j->child, w->id))
							break;
						w = w->next;
					}

					if(!w)				/* no double properties */
					{					/* -> nodes may be merged and moved */
						success++;
						fault--;
					}
				}
			}
		}
	}

	if(fault && success)		/* critical case */
	{
		PrintError(W_VARLEVEL_UNCERTAIN, sgfc, n->row, n->col);
		return;
	}

	if(success)					/* found variations which can be corrected */
	{
		PrintError(W_VARLEVEL_CORRECTED, sgfc, n->row, n->col);

		i = n->sibling;
		while(i)
		{
			b = i->prop;
			while(b)			/* move !SETUP properties to second node */
			{
				w = b->next;

				if(!(b->flags & TYPE_SETUP))
				{
					Delete(&i->prop, b);
					Enqueue(&i->child->prop, b);
				}

				b = w;
			}

			j = i->child;
			i->child = NULL;
			i = i->sibling;

			DelNode(sgfc, j->parent, E_NO_ERROR);	/* delete SETUP node */

			j->parent = p->parent;					/* move tree to new level */
			k = p;
			while(k->sibling)
				k = k->sibling;
			k->sibling = j;
		}
	}
}

/**************************************************************************
*** Function:	CorrectVariations
***				Checks for wrong variation levels and corrects them
*** Parameters: sgfc ... pointer to SGFInfo structure
***				r	 ... start node
***				ti	 ... pointer to TreeInfo (for check of GM)
*** Returns:	-
**************************************************************************/

static void CorrectVariations(struct SGFInfo *sgfc, struct Node *r, struct TreeInfo *ti)
{
	struct Node *n;

	if(!r)
		return;

	if(!r->parent)		/* root node? */
	{
		n = r;
		while(n)
		{
			if(FindProperty(n, TKN_B) || FindProperty(n, TKN_W))
			{
				SplitNode(sgfc, n, TYPE_ROOT | TYPE_GINFO, TKN_NONE, false);
				PrintError(WS_MOVE_IN_ROOT, sgfc, n->row, n->col);
			}
			n = n->sibling;
		}
	}

	if(ti->GM != 1)		/* variation level correction only for Go games */
		return;

	while(r)
	{
		if(r->sibling)
		{
			n = r;
			while(n)
			{
				if(!r->parent) CorrectVariations(sgfc, n->child, ti->next);
				else CorrectVariations(sgfc, n->child, ti);

				n = n->sibling;
			}

			CorrectVariation(sgfc, r);
			break;
		}

		r = r->child;
	}
}


/**************************************************************************
*** Function:	ReorderVariations
***				Reorders variations (including main branch) from A,B,C to C,B,A
*** Parameters: sgfc ... pointer to SGFInfo structure
***				r	 ... start node
*** Returns:	-
**************************************************************************/

static void ReorderVariations(struct SGFInfo *sgfc, struct Node *r)
{
	struct Node *n, *s[MAX_REORDER_VARIATIONS];
	int i;

	if(!r)
		return;

	if (!r->parent && r->sibling)
		ReorderVariations(sgfc, r->sibling);

	while(r)
	{
		if(r->child && r->child->sibling)
		{
			i = 0;
			n = r->child;
			while(n)
			{
				if(i >= MAX_REORDER_VARIATIONS)
				{
					PrintError(E_TOO_MANY_VARIATIONS, sgfc, n->row, n->col);
					break;
				}
				s[i++] = n;
				ReorderVariations(sgfc, n);
				n = n->sibling;
			}
			if(i < MAX_REORDER_VARIATIONS)
			{
				i--;
				s[0]->sibling = NULL;
				s[0]->parent->child = s[i];
				for(; i > 0; i--)
					s[i]->sibling = s[i-1];
			}
			break;
		}
		r = r->child;
	}
}


/**************************************************************************
*** Function:	DelEmptyNodes (recursive)
***				Deletes empty nodes
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... start node
*** Returns:	-
**************************************************************************/

static void DelEmptyNodes(struct SGFInfo *sgfc, struct Node *n)
{
	if(n->child)
		DelEmptyNodes(sgfc, n->child);

	if(n->sibling)
		DelEmptyNodes(sgfc, n->sibling);

	if(!n->prop)
		DelNode(sgfc, n, W_EMPTY_NODE_DELETED);
}


/**************************************************************************
*** Function:	SplitNode
***				Splits one node into two and moves selected properties
***				into the second node
*** Parameters: sgfc	... pointer to SGFInfo structure
***				n		... node that should be split
***				flags	... prop->flags to select properties
***				id		... id of an extra property
***				move	... true:  move selected to second node
***							false: selected props stay in first node
*** Returns:	-
**************************************************************************/

void SplitNode(struct SGFInfo *sgfc, struct Node *n, U_SHORT flags, token id, bool move)
{
	struct Property *p, *hlp;
	struct Node *newnode;

	newnode = NewNode(sgfc, n, true);		/* create new child node */
	newnode->row = n->row;
	newnode->col = n->col;

	p = n->prop;
	while(p)
	{
		hlp = p->next;
		if((move && ((p->flags & flags) || p->id == id)) ||
		  (!move && (!(p->flags & flags) && p->id != id)))
		{
			Delete(&n->prop, p);
			AddTail(&newnode->prop, p);
		}
		p = hlp;
	}
}


/**************************************************************************
*** Function:	SplitMoveSetup
***				Tests if node contains move & setup properties
***				if yes -> splits node into two: setup & N[] --- other props
***				Deletes PL[] property if it's the only setup property
***				(frequent error of an application (which one?))
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... pointer to Node
*** Returns:	true if node is split / false otherwise
**************************************************************************/

static int SplitMoveSetup(struct SGFInfo *sgfc, struct Node *n)
{
	struct Property *p, *s = NULL;
	U_SHORT f, sc = 0;

	p = n->prop;
	f = 0;

	while(p)				/* OR all flags */
	{
		if(p->flags & TYPE_SETUP)
		{
			s = p;			/* remember property */
			sc++;			/* setup count++ */
		}

		f |= p->flags;
		p = p->next;
	}

	if((f & TYPE_SETUP) && (f & TYPE_MOVE))		/* mixed them? */
	{
		if(sc == 1 && s->id == TKN_PL)			/* single PL[]? */
		{
			PrintError(E4_MOVE_SETUP_MIXED, sgfc, s->row, s->col, "deleted PL property");
			DelProperty(n, s);
		}
		else
		{
			PrintError(E4_MOVE_SETUP_MIXED, sgfc, s->row, s->col, "split into two nodes");
			SplitNode(sgfc, n, TYPE_SETUP | TYPE_GINFO | TYPE_ROOT, TKN_N, false);
			return true;
		}
	}
	return false;
}


/**************************************************************************
*** Function:	CheckDoubleProp
***				Checks uniqueness of properties within a node
***				Tries to merge or link values, otherwise deletes property
***				- can't merge !PVT_LIST && PVT_COMPOSE
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... pointer to Node
*** Returns:	-
**************************************************************************/

static void CheckDoubleProp(struct SGFInfo *sgfc, struct Node *n)
{
	struct Property *p, *q;
	struct PropValue *v, *w;
	char *c;
	size_t l;

	p = n->prop;
	while(p)
	{
		q = p->next;
		while(q)
		{
			/* ID's identical? stridcmp() for TKN_UNKNOWN */
			if(p->id == q->id && !stridcmp(p->idstr, q->idstr))
			{
				if(p->flags & DOUBLE_MERGE)
				{
					PrintError(E_DOUBLE_PROP, sgfc, q->row, q->col, q->idstr, "values merged");
					if(p->flags & PVT_LIST)
					{
						v = p->value;
						while(v->next)	v = v->next;
						v->next = q->value;
						q->value->prev = v;
						p->valend = q->valend;
						q->value = NULL;	/* values are not deleted */
						q->valend = NULL;
					}
					else	/* single values are merged to one value */
					{
						v = p->value;	l = strlen(v->value);
						w = q->value;

						SaveMalloc(char *, c, l + strlen(w->value) + 4, "new property value")

						strcpy(c, v->value);
						strcpy(c+l+2, w->value);
						c[l]   = '\n';
						c[l+1] = '\n';

						free(v->value);		/* free old buffer */
						v->value = c;
					}
				}
				else
					PrintError(E_DOUBLE_PROP, sgfc, q->row, q->col, q->idstr, "deleted");

				q = DelProperty(n, q);	/* delete double property */
			}
			else
				q = q->next;
		}
		p = p->next;
	}
}


/**************************************************************************
*** Function:	GetNumber
***				Parses a positive int value for correctness
***				deletes property otherwise
*** Parameters: sgfc  ... pointer to SGFInfo structure
***				n	  ... pointer to node that contains property
***				p	  ... pointer to property
***				value ... 2: parse value2 else parse value1
***				d	  ... pointer to int variable
***				def   ... default return value
*** Returns:	true ... success / false ... erroneous property deleted
**************************************************************************/

static bool GetNumber(struct SGFInfo *sgfc, struct Node *n, struct Property *p,
					 int value, int *d, int def, char *err_action)
{
	char *v;

	if(!p)				/* no property? -> set default value */
	{
		*d = def;
		return true;
	}

	if(value == 2)	v = p->value->value2;
	else			v = p->value->value;

	switch(Parse_Number(v))
	{
		case 0: PrintError(E_BAD_ROOT_PROP, sgfc, p->value->row, p->value->col, p->idstr, err_action);
				*d = def;
				DelProperty(n, p);
				return false;

		case -1: PrintError(E_BAD_VALUE_CORRECTED, sgfc, p->value->row, p->value->col,
					  		p->value->value, p->idstr, v);
		case 1:	*d = atoi(v);
				if(*d < 1)
				{
					PrintError(E_BAD_ROOT_PROP, sgfc, p->value->row, p->value->col, p->idstr, err_action);
					*d = def;
					DelProperty(n, p);
					return false;
				}
				break;
	}

	return true;
}


/**************************************************************************
*** Function:	InitTreeInfo
***				Inits new TreeInfo structure base on root node "r"
*** Parameters: sgfc ... pointer to SGFInfo structure
***				ti	 ... pointer to tree info to initialize
***				r	 ... pointer to root node
*** Returns:	-
**************************************************************************/

static void InitTreeInfo(struct SGFInfo *sgfc, struct TreeInfo *ti, struct Node *r)
{
	struct Property *ff, *gm, *sz;

	ti->FF = 0;						/* Init structure */
	ti->GM = 0;
	ti->bwidth = ti->bheight = 0;
	ti->root = r;
	if(sgfc->last)
		ti->num = sgfc->last->num + 1;
	else
		ti->num = 1;

	ff = FindProperty(r, TKN_FF);
	if(!GetNumber(sgfc, r, ff, 1, &ti->FF, 1, "FF[1]"))
		ff = NULL;

	if(ti->FF > 4)
		PrintError(E_UNKNOWN_FILE_FORMAT, sgfc, ff->value->row, ff->value->col, ti->FF);

	gm = FindProperty(r, TKN_GM);
	GetNumber(sgfc, r, gm, 1, &ti->GM, 1, "GM[1]");
	if(ti->GM != 1)
	{
		PrintError(WCS_GAME_NOT_GO, sgfc, gm->row, gm->col, ti->num);
		return;		/* board size only of interest if Game == Go */
	}

	sz = FindProperty(r, TKN_SZ);
	if(!sz)
	{
		ti->bwidth = ti->bheight = 19;	/* default size */
		return;
	}

	if(!GetNumber(sgfc, r, sz, 1, &ti->bwidth, 19, "19x19"))
	{
		/* faulty SZ deleted, ti->bwidth set by GetNumber to default 19 */
		ti->bheight = 19;
		return;
	}

	if(ti->FF < 4 && (ti->bwidth > 19 || sz->value->value2))
		PrintError(E_VERSION_CONFLICT, sgfc, sz->row, sz->col, ti->FF);

	if(sz->value->value2)	/* rectangular board? */
	{
		if(!GetNumber(sgfc, r, sz, 2, &ti->bheight, 19, "19x19"))
			ti->bwidth = 19;
		else
		{
			if(ti->bwidth == ti->bheight)
			{
				PrintError(E_SQUARE_AS_RECTANGULAR, sgfc, sz->row, sz->col);
				free(sz->value->value2);
				sz->value->value2 = NULL;
			}
		}
	}
	else			/* square board */
		ti->bheight = ti->bwidth;

	if(ti->bheight > 52 || ti->bwidth > 52)	/* board too big? */
	{
		if(ti->bwidth > 52)
		{
			ti->bwidth = 52;
			strcpy(sz->value->value, "52");
		}

		if(ti->bheight > 52)
		{
			ti->bheight = 52;
			if(sz->value->value2)
				strcpy(sz->value->value2, "52");
		}

		if(ti->bwidth == ti->bheight && sz->value->value2)
		{
			free(sz->value->value2);
			sz->value->value2 = NULL;
		}

		PrintError(E_BOARD_TOO_BIG, sgfc, sz->row, sz->col, ti->bwidth, ti->bheight);
	}
}

void InitAllTreeInfo(struct SGFInfo *sgfc)
{
	struct Node *root = sgfc->first;
	struct TreeInfo *ti;

	for(; root; root = root->sibling)
	{
		SaveMalloc(struct TreeInfo *, ti, sizeof(struct TreeInfo), "tree info structure")
		InitTreeInfo(sgfc, ti, root);
		AddTail(&sgfc->tree, ti);		/* add to SGFInfo */
	}
}

void CheckDifferingFFAndGM(struct SGFInfo *sgfc)
{
	struct TreeInfo *ti = sgfc->tree->next;
	struct Property *gm, *ff;
	int gm_val, ff_val;
	U_LONG row, col;

	for(; ti; ti = ti->next)
	{
		ff = FindProperty(ti->root, TKN_FF);
		GetNumber(sgfc, ti->root, ff, 1, &ff_val, 1, "FF[1]");
		gm = FindProperty(ti->root, TKN_GM);
		GetNumber(sgfc, ti->root, gm, 1, &gm_val, 1, "FF[1]");

		if(ti->prev->FF != ti->FF)
		{
			if(ff)	{ row = ff->row;		col = ff->col; }
			else	{ row = ti->root->row;	col = ti->root->col; }

			PrintError(WS_FF_DIFFERS, sgfc, row, col);
		}

		if(ti->prev->GM != ti->GM)
		{
			if(gm)	{ row = gm->row;		col = gm->col; }
			else	{ row = ti->root->row;	col = ti->root->col; }

			PrintError(WS_GM_DIFFERS, sgfc, row, col);
		}
	}
}

/**************************************************************************
*** Function:	CheckSGFTree
***				Steps recursive through the SGF tree and
***				calls Check_Properties for each node
*** Parameters: sgfc ... pointer to SGFInfo structure
***				r    ... pointer to root node of current tree
***				old  ... board status before parsing root node
*** Returns:	-
**************************************************************************/

static void CheckSGFSubTree(struct SGFInfo *sgfc, struct Node *r, struct BoardStatus *old)
{
	struct Node *n;
	struct BoardStatus *st;
	int area;

	SaveMalloc(struct BoardStatus *, st, sizeof(struct BoardStatus), "board status buffer")

	while(r)
	{
		memcpy(st, old, sizeof(struct BoardStatus));
		area = old->bwidth * old->bheight;
		if(st->board)
		{
			SaveMalloc(unsigned char *, st->board, area * sizeof(char), "goban buffer")
			memcpy(st->board, old->board, area * sizeof(char));
		}
		/* path_board is reused (paths marked with different path_num) */
		/* markup is reused (set to 0 for each new node) */
		st->markup_changed = true;

		n = r;
		while(n)
		{
			st->annotate = 0;
			if(st->markup_changed && st->markup)
				memset(st->markup, 0, area * sizeof(U_SHORT));
			st->markup_changed = false;

			if(n->sibling && n != r)		/* for n=r loop is done outside */
			{
				CheckSGFSubTree(sgfc, n, st);
				break;						/* did complete subtree -> break */
			}

			CheckDoubleProp(sgfc, n);		/* remove/merge double properties */
			Check_Properties(sgfc, n, st);	/* perform checks, update board status */

			if(SplitMoveSetup(sgfc, n))
				n = n->child;				/* new child node already parsed */

			n = n->child;
		}

		if(st->board)
			free(st->board);
		if(!r->parent)
			break;
		r = r->sibling;
	}

	free(st);
}

static void CheckSGFTree(struct SGFInfo *sgfc, struct TreeInfo *ti)
{
	struct Node *n;
	struct BoardStatus *st;
	int area;

	SaveMalloc(struct BoardStatus *, st, sizeof(struct BoardStatus), "board status buffer")

	while(ti)
	{
		sgfc->info = ti;
		memset(st, 0, sizeof(struct BoardStatus));
		st->bwidth = sgfc->info->bwidth;
		st->bheight = sgfc->info->bheight;
		area = st->bwidth * st->bheight;
		if(area)
		{
			SaveMalloc(unsigned char *, st->board, area * sizeof(char), "goban buffer")
			memset(st->board, 0, area * sizeof(char));
			SaveMalloc(U_SHORT *, st->markup, area * sizeof(U_SHORT), "markup buffer")
			SaveMalloc(struct PathBoard *, st->paths, sizeof(struct PathBoard), "path_board buffer")
			memset(st->paths, 0, sizeof(struct PathBoard));
		}
		st->markup_changed = true;

		CheckSGFSubTree(sgfc, ti->root, st);

		ti = ti->next;

		if(st->board)	free(st->board);
		if(st->markup)	free(st->markup);
		if(st->paths)	free(st->paths);
	}

	free(st);
}


/**************************************************************************
*** Function:	ParseSGF
***				Calls the check routines one after another
*** Parameters: sgfc ... pointer to SGFInfo structure
*** Returns:	-
**************************************************************************/

void ParseSGF(struct SGFInfo *sgfc)
{
	InitAllTreeInfo(sgfc);

	CheckSGFTree(sgfc, sgfc->tree);
	CheckDifferingFFAndGM(sgfc);

	if(sgfc->options->fix_variation)
		CorrectVariations(sgfc, sgfc->root, sgfc->tree);

	if(sgfc->options->del_empty_nodes)
		DelEmptyNodes(sgfc, sgfc->root);

	if(sgfc->options->reorder_variations)
		ReorderVariations(sgfc, sgfc->root);

	if(sgfc->options->strict_checking)
		StrictChecking(sgfc);
}
