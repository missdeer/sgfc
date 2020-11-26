/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 load.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
*** Notes:	Almost all routines in this file return either
***			- false (NULL)	for reaching the end of file (UNEXPECTED_EOF)
***			- true (value)	for success (or for: 'continue with parsing')
***			- exit program on a fatal error (e.g. if malloc() fails)
*** 		Almost all routines get passed a current SGFInfo structure
***			and read/modify sgfc->current
**************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "all.h"
#include "protos.h"

#define SGF_EOF			(sgfc->current >= sgfc->b_end)

/* defines for SkipText */
#define INSIDE	0u
#define OUTSIDE 1u
#define P_ERROR	2u


/**************************************************************************
*** Function:	NextCharInBuffer
***				Advanced buffer pointer and keep track of row & column number
***				Counts \r\n or \n\r as single step.
*** Parameters: c	 ... current position
***				end  ... end position of buffer
***				step ... how many steps to take
***				row	 ... row number associated with c
***				col	 ... column associated with c
*** Returns:	current position; row & col are updated accordingly
**************************************************************************/

static const char *NextCharInBuffer(const char **c, const char *end, U_LONG step, U_LONG *row, U_LONG *col)
{
	for(; step > 0 && *c < end; step--)
	{
		if(**c == '\r' || **c == '\n')		/* linebreak char? */
		{
			if(row)
			{
				(*row)++;
				*col = 1;
			}
			/* next char is linebreak too, but different from current char? */
			if(*c+1<end && (*(*c+1) == '\r' || *(*c+1) == '\n') && *(*c+1) != **c)
				(*c)++;						/* ->yes: skip, no real linebreak */
		}
		else								/* no linebreak char */
			if(col)
				(*col)++;
		(*c)++;
	}
	return (*c);
}


/**************************************************************************
*** Function:	NextChar
***				Convience wrapper for NextCharInBuffer
*** Parameters: sfgc ... pointer to SGFInfo
*** Returns:	current position; row & col are updated accordingly
**************************************************************************/

static const char *NextChar(struct SGFInfo *sgfc)
{
	return NextCharInBuffer(&sgfc->current, sgfc->b_end, 1, &sgfc->cur_row, &sgfc->cur_col);
}


/**************************************************************************
*** Function:	SkipText
***				Skips all chars until break char is detected or
***				end of buffer is reached
*** Parameters: sgfc	... pointer to SGFInfo structure
***				s		... pointer to buffer start
***				e		... pointer to buffer end
***							(may be NULL -> buffer terminated with '\0')
***				end		... break char
***				mode	... INSIDE  : do escaping ('\')
***							OUTSIDE : detect faulty chars
***							P_ERROR : print UNEXPECTED_EOF error message
***				row		... row number associated with 's'
***				col		... column number associated with 's'
*** Returns:	pointer to break char or NULL if buffer end was reached.
***				(in case of NULL, row & col reflect position at buffer end)
**************************************************************************/

static const char *SkipText(struct SGFInfo *sgfc, const char *s, const char *e,
							char end, unsigned int mode, U_LONG *row, U_LONG *col)
{
	while(s < e)
	{
		if(*s == end)			/* found break char? */
			return s;

		if(mode & OUTSIDE)		/* '.. [] ..' */
		{
			if(!isspace(*s))
				PrintError(E_ILLEGAL_OUTSIDE_CHAR, sgfc, *row, *col, true, s);
		}
		else					/* '[ .... ]' */
		{
			if(*s == '\\')		/* escaping */
			{
				NextCharInBuffer(&s, e, 2, row, col);
				continue;
			}
		}
		NextCharInBuffer(&s, e, 1, row, col);
	}

	if(mode & P_ERROR)
		PrintError(E_UNEXPECTED_EOF, sgfc, *row, *col);

	return NULL;
}


/**************************************************************************
*** Function:	SkipSGFText
***				Wrapper for SkipText, using sgfc->current as buffer
*** Parameters: sgfc	... pointer to SGFInfo structure
***				brk		... break char
***				mode	... see SkipText
*** Returns:	true or false
**************************************************************************/

static bool SkipSGFText(struct SGFInfo *sgfc, char brk, unsigned int mode)
{
	const char *pos = SkipText(sgfc, sgfc->current, sgfc->b_end,
							   brk, mode, &sgfc->cur_row, &sgfc->cur_col);

	sgfc->lowercase = 0;		/* we are no longer parsing for GetNextSGFChar -> reset */

	/* Reached end of buffer? */
	if (!pos)
	{
		sgfc->current = sgfc->b_end;	/* row & col already updated by SkipText */
		return false;
	}

	sgfc->current = pos;
	return true;
}


/**************************************************************************
*** Function:	GetNextSGFChar
***				Sets sgfc->current to next meaningful SGF char
***				Detects bad chars and prints an error message if desired
***				Chars: ( ) ; [ uppercase
***					In last case sgfc->current points to beginning of text
***				 	(leading lowercase possible)
*** Parameters: sgfc		... pointer to SGFInfo structure
***				print_error ... print error message
***				error		... error code for printing on failure (or E_NO_ERROR)
*** Returns:	true or false
**************************************************************************/

static bool GetNextSGFChar(struct SGFInfo *sgfc, bool print_error, U_LONG error)
{
	U_LONG lc = 0;

	while(!SGF_EOF)
	{
		switch(*sgfc->current)
		{
			case ';':
			case '(':
			case ')':
			case '[':	if(print_error && lc)
							PrintError(E_ILLEGAL_OUTSIDE_CHARS, sgfc, sgfc->cur_row, sgfc->cur_col-lc,
				  				       true, sgfc->current-lc, lc);
						sgfc->lowercase = 0;
						return true;

			default:	if(isupper(*sgfc->current))
						{
							sgfc->lowercase += lc;
							return true;
						}
						if(islower(*sgfc->current))
							lc++;
						else		/* !islower && !isupper */
						{
							if(print_error)
							{
								if(lc)
									PrintError(E_ILLEGAL_OUTSIDE_CHARS, sgfc, sgfc->cur_row, sgfc->cur_col-lc,
											   true, sgfc->current-lc, lc);
								if(!isspace(*sgfc->current))
									PrintError(E_ILLEGAL_OUTSIDE_CHAR, sgfc, sgfc->cur_row, sgfc->cur_col,
											   true, sgfc->current);
							}
							lc = 0;
							sgfc->lowercase = 0;
						}
						NextChar(sgfc);
						break;
		}
	}

	if(error != E_NO_ERROR)
		PrintError(error, sgfc, sgfc->cur_row, sgfc->cur_col);
	sgfc->lowercase = 0;
	return false;
}


/**************************************************************************
*** Function:	SkipValues
***				Skips all property values of current value list
*** Parameters: sgfc		... pointer to SGFInfo structure
***				print_error ... print error message
***								(passed on to GetNextSGFChar)
*** Returns:	true or false
**************************************************************************/

static bool SkipValues(struct SGFInfo *sgfc, bool print_error)
{
	if(!SkipSGFText(sgfc, '[', OUTSIDE|P_ERROR))	/* search start of first value */
		return false;

	while(*sgfc->current == '[')
	{
		if(!SkipSGFText(sgfc, ']', INSIDE|P_ERROR))	/* skip value */
			return false;

		NextChar(sgfc);

		/* search next value start */
		if(!GetNextSGFChar(sgfc, print_error, E_UNEXPECTED_EOF))
			return false;
	}

	return true;
}


/**************************************************************************
*** Function:	NewValue
***				Adds one property value to the given property
*** Parameters: sgfc	... pointer to SGFInfo structure
***				p		... pointer to property
***				flags	... property flags (as in sgf_token[])
*** Returns:	true or false
**************************************************************************/

static bool NewValue(struct SGFInfo *sgfc, struct Property *p, U_SHORT flags)
{
	U_LONG row = sgfc->cur_row;
	U_LONG col = sgfc->cur_col;

	const char *s = NextChar(sgfc);		/* points to char after '[' */
	if(!s)
		return false;

	if(!SkipSGFText(sgfc, ']', INSIDE|P_ERROR))
		return false;					/* value isn't added */

	NextChar(sgfc);						/* points now to char after ']' */

	if(flags & (PVT_COMPOSE|PVT_WEAKCOMPOSE))	/* compose datatype? */
	{
		const char *t = SkipText(sgfc, s, sgfc->current, ':', INSIDE, NULL, NULL);
		if(!t)
		{
			if(flags & PVT_WEAKCOMPOSE)	/* no compose -> parse as normal */
				AddPropValue(sgfc, p, row, col, s, sgfc->current - s - 1, NULL, 0);
			else						/* not weak -> error */
			{
				char *val = SaveDupString(s, sgfc->current - s - 1, "compose error value");
				PrintError(E_COMPOSE_EXPECTED, sgfc, row, col, val, p->idstr);
				free(val);
			}
		}
		else	/* composed value */
			AddPropValue(sgfc, p, row, col, s, t - s, t + 1, sgfc->current - t - 2);
	}
	else
		AddPropValue(sgfc, p, row, col, s, sgfc->current - s - 1, NULL, 0);

	return true;
}


/**************************************************************************
*** Function:	NewProperty
***				Adds one property (id given) to a node
*** Parameters: sgfc	... pointer to SGFInfo structure
***				n		... node to which property belongs to
***				id		... tokenized ID of property
***				id_buf	... pointer to property ID
***				idstr	... ID string
*** Returns:	true or false
**************************************************************************/

static bool NewProperty(struct SGFInfo *sgfc, struct Node *n, token id, U_LONG row, U_LONG col, char *idstr)
{
	struct Property *newp;
	bool ret = true;
	U_LONG tooMany_row = 0, tooMany_col;

	if(!n)	return true;

	newp = AddProperty(n, id, row, col, idstr);

	while(true)
	{
		if(!NewValue(sgfc, newp, newp->flags))	/* add value */
		{
			ret = false;	break;
		}

		if(!GetNextSGFChar(sgfc, true, E_VARIATION_NESTING))
		{
			ret = false;	break;
		}

		if(*sgfc->current == '[')	/* more than one value? */
		{
			if(newp->flags & PVT_LIST)
				continue;
			/* error, as only one value allowed */
			if (!tooMany_row)
			{
				tooMany_row = sgfc->cur_row;
				tooMany_col = sgfc->cur_col;
			}
			if (!newp->value || !newp->value->value_len)	/* if previous value is empty, */
			{												/* then use the later value */
				DelPropValue(newp, newp->value);
				continue;
			}
			SkipValues(sgfc, false);
			break;
		}
		break;						/* reached end of value list */
	}

	if(tooMany_row)
		PrintError(E_TOO_MANY_VALUES, sgfc, tooMany_row, tooMany_col, idstr);

	if(!newp->value)				/* property has values? */
		DelProperty(n, newp);		/* no -> delete it */

	return ret;
}


/**************************************************************************
*** Function:	MakeProperties
***				builds property-list from a given SGF string
*** Parameters: sgfc ... pointer to SGFInfo structure
***				n	 ... node to which properties should belong
*** Returns:	true or false
**************************************************************************/

static bool MakeProperties(struct SGFInfo *sgfc, struct Node *n)
{
	char propid[100], full_propid[300];
	U_LONG id_row, id_col;
	int pi, pi_lc;

	while(true)
	{
		if(!GetNextSGFChar(sgfc, true, E_VARIATION_NESTING))
			return false;

		switch(*sgfc->current)
		{
			case '(':	/* ( ) ; indicate node end */
			case ')':
			case ';':	return true;
			case ']':	PrintError(E_ILLEGAL_OUTSIDE_CHAR, sgfc, sgfc->cur_row, sgfc->cur_col, true, sgfc->current);
						NextChar(sgfc);
						break;
			case '[':	PrintError(E_VALUES_WITHOUT_ID, sgfc, sgfc->cur_row, sgfc->cur_col);
						if(!SkipValues(sgfc, true))
							return false;
						break;

			default:	/* isalpha */
				id_row = sgfc->cur_row;
				id_col = sgfc->cur_col;
				pi = 0;		/* counter for propid */
				pi_lc = 0;	/* counter for lowercase propid */

				if(sgfc->lowercase)
				{
					U_LONG lc = sgfc->lowercase >= 200 ? 199 : sgfc->lowercase;
					strncpy(full_propid, sgfc->current - sgfc->lowercase, lc);
					pi_lc = lc;
					id_col -= sgfc->lowercase;
				}

				while(!SGF_EOF)
				{
					if(islower(*sgfc->current))
					{
						if(pi_lc < 200)
						{
							full_propid[pi + pi_lc] = *sgfc->current;
							pi_lc++;
						}
					}
					else if(isupper(*sgfc->current))
					{
						if(pi < 100)						/* max. 100 uc chars */
						{
							full_propid[pi+pi_lc] = *sgfc->current;
							propid[pi++] = *sgfc->current;
						}
					}
					else									/* end of PropID? */
					{
						propid[pi >= 100 ? 99 : pi] = 0;
						full_propid[pi+pi_lc >= 300 ? 299 : pi+pi_lc] = 0;

						if(pi >= 100)
							break;

						if(!GetNextSGFChar(sgfc, true, E_UNEXPECTED_EOF))
							return false;

						if(*sgfc->current != '[')
						{
							PrintError(E_NO_PROP_VALUES, sgfc, id_row, id_col, full_propid);
							break;
						}

						if(pi > 2)
							PrintError(WS_LONG_PROPID, sgfc, sgfc->cur_row, sgfc->cur_col, full_propid);

						int i = 1;
						for(; sgf_token[i].id; i++)
							if(!strcmp(propid, sgf_token[i].id))
								break;

						if(!sgf_token[i].id)	/* EOF sgf_token */
						{
							if(!sgfc->options->keep_unknown_props)
							{
								PrintError(WS_UNKNOWN_PROPERTY, sgfc, id_row, id_col, full_propid, "deleted");
								if(!SkipValues(sgfc, true))
									return false;
								break;
							}
							PrintError(WS_UNKNOWN_PROPERTY, sgfc, id_row, id_col, full_propid, "found");
							i = TKN_UNKNOWN;
						}

						if(sgfc->options->delete_property[i])
						{
							PrintError(W_PROPERTY_DELETED, sgfc, id_row, id_col, "", full_propid);
							if(!SkipValues(sgfc, true))
								return false;
							break;
						}

						if(!NewProperty(sgfc, n, (token)i, id_row, id_col, full_propid))
							return false;
						break;
					}
					NextChar(sgfc);
				}

				if(SGF_EOF)
				{
					PrintError(E_UNEXPECTED_EOF, sgfc, sgfc->cur_row, sgfc->cur_col);
					return false;
				}

				if(pi >= 100)
				{
					PrintError(E_PROPID_TOO_LONG, sgfc, id_row, id_col, full_propid);
					if(!SkipValues(sgfc, true))
						return false;
				}
				break;
		}
	}
}


/**************************************************************************
*** Function:	NewNodeWithProperties
***				Small helper function for creating node and parsing properties
*** Parameters: sgfc 	... pointer to SGFInfo structure
***				parent	... parent node
*** Returns:	pointer to Node or NULL
**************************************************************************/

static struct Node *NewNodeWithProperties(struct SGFInfo *sgfc, struct Node *parent)
{
	struct Node *n = NewNode(sgfc, parent, false);

	if(!MakeProperties(sgfc, n))
		return NULL;

	return n;
}


/**************************************************************************
*** Function:	BuildSGFTree
***				Recursive function to build up the sgf tree structure
*** Parameters: sgfc ... pointer to SGFInfo structure
***				r	 ... tree root
***				missing_semicolon ... whether missing semicolon is known/reported already
*** Returns:	true or false on success/error
**************************************************************************/

static bool BuildSGFTree(struct SGFInfo *sgfc, struct Node *r, bool missing_semicolon)
{
	int end_tree = 0, empty = 1;

	while(GetNextSGFChar(sgfc, true, E_VARIATION_NESTING))
	{
		switch(*sgfc->current)
		{
			case ';':	if(end_tree)
						{
							PrintError(E_NODE_OUTSIDE_VAR, sgfc, sgfc->cur_row, sgfc->cur_col);
							if(!BuildSGFTree(sgfc, r, false))
								return false;
							end_tree = 1;
						}
						else
						{
							empty = 0;
							NextChar(sgfc);
							r = NewNodeWithProperties(sgfc, r);
							if(!r)
								return false;
						}
						break;
			case '(':	if(empty)
						{
							if(!missing_semicolon)
								PrintError(E_VARIATION_START, sgfc, sgfc->cur_row, sgfc->cur_col);
							NextChar(sgfc);
						}
						else
						{
							NextChar(sgfc);
							if(!BuildSGFTree(sgfc, r, false))
								return false;
							end_tree = 1;
						}
						break;
			case ')':	if(empty)
							PrintError(E_EMPTY_VARIATION, sgfc, sgfc->cur_row, sgfc->cur_col);
						NextChar(sgfc);
						return true;

			default:	if(empty)		/* assume there's a missing ';' */
						{
							if(!missing_semicolon)
								PrintError(E_MISSING_NODE_START, sgfc,
				   						   sgfc->cur_row, sgfc->cur_col - sgfc->lowercase);
							empty = 0;
							r = NewNodeWithProperties(sgfc, r);
							if(!r)
								return false;
						}
						else
						{
							PrintError(E_ILLEGAL_OUTSIDE_CHARS, sgfc,
				  					   sgfc->cur_row, sgfc->cur_col - sgfc->lowercase,
									   true, sgfc->current, sgfc->lowercase+1);
							NextChar(sgfc);
						}
						break;
		}
	}

	return false;
}


/**************************************************************************
*** Function:	FindStart
***				sets sgfc->current to '(' of start mark '(;'
*** Parameters: sgfc	   ... pointer to SGFInfo structure
***				first_time ... search for the first time?
***							   (true -> if search fails -> fatal error)
*** Returns:	0 ... ok / 1 ... missing ';'  / -1 ... fatal error
**************************************************************************/

static int FindStart(struct SGFInfo *sgfc, bool first_time)
{
	int warn = 0, o, c;
	const char *tmp;

	while(!SGF_EOF)
	{
		/* search for '[' (lc) (lc) ']' */
		if((sgfc->current + 4 <= sgfc->b_end) &&
		  (*sgfc->current == '['))
			if(islower(*(sgfc->current+1)) && islower(*(sgfc->current+2)) &&
			  (*(sgfc->current+3) == ']'))
			{
				if(!warn)		/* print warning only once */
				{
					PrintError(W_SGF_IN_HEADER, sgfc, sgfc->cur_row, sgfc->cur_col);
					warn = 1;
				}

				if(!first_time)
					PrintError(E_ILLEGAL_OUTSIDE_CHARS, sgfc, sgfc->cur_row, sgfc->cur_col, true, sgfc->current, 4UL);

				sgfc->current += 4;	/* skip '[aa]' */
				continue;
			}

		if(*sgfc->current == '(')	/* test for start mark '(;' */
		{
			tmp = sgfc->current + 1;
			while((tmp < sgfc->b_end) && isspace(*tmp))
				tmp++;

			if(tmp == sgfc->b_end)
				break;

			if(*tmp == ';')
				return 0;

			o = c = 0;

			if(sgfc->options->find_start == OPTION_FINDSTART_SEARCH)
			{		/* found a '(' but no ';' -> might be a missing ';' */
				tmp = sgfc->current + 1;
				while((tmp != sgfc->b_end) && *tmp != ')' && *tmp != '(')
				{
					if(*tmp == '[')		o++;
					if(*tmp == ']')		c++;
					tmp++;
				}
			}

			if((sgfc->options->find_start == OPTION_FINDSTART_BRACKET) ||
			   ((o >= 2) && (o >= c) && (o-c <= 1)))
			{
				PrintError(E_MISSING_SEMICOLON, sgfc, sgfc->cur_row, sgfc->cur_col);
				return 2;
			}
		}
		else
			if(!first_time && !isspace(*sgfc->current))
				PrintError(E_ILLEGAL_OUTSIDE_CHAR, sgfc, sgfc->cur_row, sgfc->cur_col, true, sgfc->current);

		NextChar(sgfc);
	}

	if(first_time)
	{
		PrintError(FE_NO_SGFDATA, sgfc);
		return -1;
	}

	return 0;
}


/**************************************************************************
*** Function:	LoadSGF
***				Loads a SGF file into the memory and inits all
***				necessary information in sgfinfo-structure
*** Parameters: sgfc ... pointer to SGFInfo structure
***				name ... filename/path
*** Returns:	true on success, false on fatal error
**************************************************************************/

bool LoadSGF(struct SGFInfo *sgfc, const char *name)
{
	long size;
	FILE *file;

	file = fopen(name, "rb");
	if(!file)
	{
		PrintError(FE_SOURCE_OPEN, sgfc, name);
		return false;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);

	if(size == -1L)
		goto load_error;

	sgfc->buffer = (char *) malloc((size_t) size);
	if(!sgfc->buffer)
	{
		fclose(file);
		PrintError(FE_OUT_OF_MEMORY, sgfc, "source file buffer");
		return false;
	}

	if(fseek(file, 0, SEEK_SET) == -1L)
		goto load_error;
	if(size != (long)fread(sgfc->buffer, 1, (size_t)size, file))
		goto load_error;

	sgfc->b_end   = sgfc->buffer + size;
	fclose(file);

	return LoadSGFFromFileBuffer(sgfc);

load_error:
	fclose(file);
	PrintError(FE_SOURCE_READ, sgfc, name);
	return false;
}


/**************************************************************************
*** Function:	LoadSGFFromFileBuffer
***				Seeks start of SGF data and builds basic tree structure
***             Assumes sgf->buffer and sgf->b_end is already set
*** Parameters: sgfc ... pointer to SGFInfo structure
*** Returns:	true on success, false on fatal error
**************************************************************************/

bool LoadSGFFromFileBuffer(struct SGFInfo *sgfc)
{
	sgfc->current = sgfc->buffer;
	sgfc->cur_row = 1;
	sgfc->cur_col = 1;

	int miss = FindStart(sgfc, true);	/* skip junk in front of '(;' */
	if(miss == -1)
		return false;

	sgfc->start = sgfc->current;

	while(!SGF_EOF)
	{
		if(!miss)
			NextChar(sgfc);				/* skip '(' */
		if(!BuildSGFTree(sgfc, NULL, miss==2))
			break;
		miss = FindStart(sgfc, false);	/* skip junk in front of '(;' */
	}

	PrintError(E_NO_ERROR, sgfc);		/* flush accumulated messages */
	return true;
}
