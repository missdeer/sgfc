/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 load.c
***
*** Copyright (C) 1996-2021 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
*** Notes:	Almost all routines in this file return either
***			- false (NULL)	for reaching the end of file (UNEXPECTED_EOF)
***			- true (value)	for success (or for: 'continue with parsing')
***			- exit program on a fatal error (e.g. if malloc() fails)
*** 		Almost all routines get passed a current SGFInfo structure
***			and read/modify load->current
**************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "all.h"
#include "protos.h"

#define SGF_EOF			(load->current >= load->b_end)

/* Internal data structure for load.c functions */
struct LoadInfo
{
	struct SGFInfo *sgfc;

	char *buffer;			/* either copy of sgfc->buffer OR decoded buffer */
	const char *b_end;

	const char *current;	/* actual read position (cursor) in buffer */
	U_LONG cur_row;			/* row & column associated with current */
	U_LONG cur_col;
	U_LONG lowercase;		/* load.c: number of lowercase chars in front of propID */

	bool is_utf8;			/* if buffer is already decoded, it's in UTF-8 */
};


/* defines for SkipText */
#define INSIDE	0u
#define OUTSIDE 1u
#define P_ERROR	2u


/**************************************************************************
*** Function:	NextCharInBuffer
***				Advanced buffer pointer and keep track of row & column number
***				Counts \r\n or \n\r as single step.
*** Parameters: c		 ... current position
***				end		 ... end position of buffer
***				step	 ... how many steps to take
***				row		 ... row number associated with c
***				col		 ... column associated with c
***				is_utf8	 ... whether we should skip UTF-8 continuation bytes
*** Returns:	current position; row & col are updated accordingly
**************************************************************************/

static const char *NextCharInBuffer(const char **c, const char *end, U_LONG step,
									U_LONG *row, U_LONG *col, bool is_utf8)
{
	for(; step > 0 && *c < end; step--)
	{
		bool skip_step = false;
		if(is_utf8 && (**c & 0xc0) == 0x80)	/* skip UTF-8 continuation bytes */
		{
			while((**c & 0xc0) == 0x80 && *c < end)
				(*c)++;
			if(*c == end)
				break;
			skip_step = true;				/* because we skipped at least 1 byte already */
		}
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
			if(col && !skip_step)			/* because otherwise 1st follow-up byte is counted */
				(*col)++;
		if(!skip_step)
			(*c)++;
	}
	return (*c);
}


/**************************************************************************
*** Function:	NextChar
***				Convience wrapper for NextCharInBuffer
*** Parameters: load ... pointer to LoadInfo structure
*** Returns:	current position; row & col are updated accordingly
**************************************************************************/

static const char *NextChar(struct LoadInfo *load)
{
	return NextCharInBuffer(&load->current, load->b_end, 1,
						 	&load->cur_row, &load->cur_col, load->is_utf8);
}


/**************************************************************************
*** Function:	SkipText
***				Skips all chars until break char is detected or
***				end of buffer is reached
*** Parameters: load	... pointer to LoadInfo structure
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

static const char *SkipText(struct LoadInfo *load, const char *s, const char *e,
							char end, unsigned int mode, U_LONG *row, U_LONG *col)
{
	while(s < e)
	{
		if(*s == end)			/* found break char? */
			return s;

		if(mode & OUTSIDE)		/* '.. [] ..' */
		{
			if(!isspace((unsigned char)*s))
				PrintError(E_ILLEGAL_OUTSIDE_CHAR, load->sgfc, *row, *col, true, s);
		}
		else					/* '[ .... ]' */
		{
			if(*s == '\\')		/* escaping */
			{
				NextCharInBuffer(&s, e, 2, row, col, load->is_utf8);
				continue;
			}
		}
		NextCharInBuffer(&s, e, 1, row, col, load->is_utf8);
	}

	if(mode & P_ERROR)
		PrintError(E_UNEXPECTED_EOF, load->sgfc, *row, *col);

	return NULL;
}


/**************************************************************************
*** Function:	SkipSGFText
***				Wrapper for SkipText, using load->current as buffer
*** Parameters: load	... pointer to LoadInfo structure
***				brk		... break char
***				mode	... see SkipText
*** Returns:	true or false
**************************************************************************/

static bool SkipSGFText(struct LoadInfo *load, char brk, unsigned int mode)
{
	const char *pos = SkipText(load, load->current, load->b_end,
							   brk, mode, &load->cur_row, &load->cur_col);

	load->lowercase = 0;		/* we are no longer parsing for GetNextSGFChar -> reset */

	/* Reached end of buffer? */
	if (!pos)
	{
		load->current = load->b_end;	/* row & col already updated by SkipText */
		return false;
	}

	load->current = pos;
	return true;
}


/**************************************************************************
*** Function:	GetNextSGFChar
***				Sets load->current to next meaningful SGF char
***				Detects bad chars and prints an error message if desired
***				Chars: ( ) ; [ uppercase
***					In last case load->current points to beginning of text
***				 	(leading lowercase possible)
*** Parameters: load		... pointer to LoadInfo structure
***				print_error ... print error message
***				error		... error code for printing on failure (or E_NO_ERROR)
*** Returns:	true or false
**************************************************************************/

static bool GetNextSGFChar(struct LoadInfo *load, bool print_error, U_LONG error)
{
	U_LONG lc = 0;

	while(!SGF_EOF)
	{
		switch(*load->current)
		{
			case ';':
			case '(':
			case ')':
			case '[':	if(print_error && lc)
							PrintError(E_ILLEGAL_OUTSIDE_CHARS, load->sgfc, load->cur_row, load->cur_col-lc,
				  				       true, load->current-lc, lc);
						load->lowercase = 0;
						return true;

			default:	if(isupper((unsigned char)*load->current))
						{
							load->lowercase += lc;
							return true;
						}
						if(islower((unsigned char)*load->current))
							lc++;
						else		/* !islower && !isupper */
						{
							if(print_error)
							{
								if(lc)
									PrintError(E_ILLEGAL_OUTSIDE_CHARS, load->sgfc, load->cur_row, load->cur_col-lc,
											   true, load->current-lc, lc);
								if(!isspace((unsigned char)*load->current))
									PrintError(E_ILLEGAL_OUTSIDE_CHAR, load->sgfc, load->cur_row, load->cur_col,
											   true, load->current);
							}
							lc = 0;
							load->lowercase = 0;
						}
						NextChar(load);
						break;
		}
	}

	if(error != E_NO_ERROR)
		PrintError(error, load->sgfc, load->cur_row, load->cur_col);
	load->lowercase = 0;
	return false;
}


/**************************************************************************
*** Function:	SkipValues
***				Skips all property values of current value list
*** Parameters: load 		... pointer to LoadInfo structure
***				print_error ... print error message
***								(passed on to GetNextSGFChar)
*** Returns:	true or false
**************************************************************************/

static bool SkipValues(struct LoadInfo *load, bool print_error)
{
	if(!SkipSGFText(load, '[', OUTSIDE|P_ERROR))	/* search start of first value */
		return false;

	while(*load->current == '[')
	{
		if(!SkipSGFText(load, ']', INSIDE|P_ERROR))	/* skip value */
			return false;

		NextChar(load);

		/* search next value start */
		if(!GetNextSGFChar(load, print_error, E_UNEXPECTED_EOF))
			return false;
	}

	return true;
}


/**************************************************************************
*** Function:	NewValue
***				Adds one property value to the given property
*** Parameters: load 	... pointer to LoadInfo structure
***				p		... pointer to property
***				flags	... property flags (as in sgf_token[])
*** Returns:	true or false
**************************************************************************/

static bool NewValue(struct LoadInfo *load, struct Property *p, U_SHORT flags)
{
	U_LONG row = load->cur_row;
	U_LONG col = load->cur_col;

	const char *s = NextChar(load);		/* points to char after '[' */
	if(!s)
		return false;

	if(!SkipSGFText(load, ']', INSIDE|P_ERROR))
		return false;					/* value isn't added */

	NextChar(load);						/* points now to char after ']' */

	if(flags & (PVT_COMPOSE|PVT_WEAKCOMPOSE))	/* compose datatype? */
	{
		const char *t = SkipText(load, s, load->current, ':', INSIDE, NULL, NULL);
		if(!t)
		{
			if(flags & PVT_WEAKCOMPOSE)	/* no compose -> parse as normal */
				AddPropValue(load->sgfc, p, row, col, s, (size_t)(load->current - s - 1), NULL, 0);
			else						/* not weak -> error */
			{
				char *val = SaveDupString(s, (size_t)(load->current - s - 1), "compose error value");
				PrintError(E_COMPOSE_EXPECTED, load->sgfc, row, col, val, p->idstr);
				free(val);
			}
		}
		else	/* composed value */
			AddPropValue(load->sgfc, p, row, col, s, (size_t)(t - s), t + 1, (size_t)(load->current - t - 2));
	}
	else
		AddPropValue(load->sgfc, p, row, col, s, (size_t)(load->current - s - 1), NULL, 0);

	return true;
}


/**************************************************************************
*** Function:	NewProperty
***				Adds one property (id given) to a node
*** Parameters: load 	... pointer to LoadInfo structure
***				n		... node to which property belongs to
***				id		... tokenized ID of property
***				id_buf	... pointer to property ID
***				idstr	... ID string
*** Returns:	true or false
**************************************************************************/

static bool NewProperty(struct LoadInfo *load, struct Node *n, token id, U_LONG row, U_LONG col, char *idstr)
{
	struct Property *newp;
	bool ret = true;
	U_LONG tooMany_row = 0, tooMany_col = 0;

	if(!n)	return true;

	newp = AddProperty(n, id, row, col, idstr);

	while(true)
	{
		if(!NewValue(load, newp, newp->flags))	/* add value */
		{
			ret = false;	break;
		}

		if(!GetNextSGFChar(load, true, E_VARIATION_NESTING))
		{
			ret = false;	break;
		}

		if(*load->current == '[')	/* more than one value? */
		{
			if(newp->flags & PVT_LIST)
				continue;
			/* error, as only one value allowed */
			if (!tooMany_row)
			{
				tooMany_row = load->cur_row;
				tooMany_col = load->cur_col;
			}
			if (!newp->value || !newp->value->value_len)	/* if previous value is empty, */
			{												/* then use the later value */
				DelPropValue(newp, newp->value);
				continue;
			}
			SkipValues(load, false);
			break;
		}
		break;						/* reached end of value list */
	}

	if(tooMany_row)
		PrintError(E_TOO_MANY_VALUES, load->sgfc, tooMany_row, tooMany_col, idstr);

	if(!newp->value)				/* property has values? */
		DelProperty(n, newp);		/* no -> delete it */

	return ret;
}


/**************************************************************************
*** Function:	MakeProperties
***				builds property-list from a given SGF string
*** Parameters: load ... pointer to LoadInfo structure
***				n	 ... node to which properties should belong
*** Returns:	true or false
**************************************************************************/

static bool MakeProperties(struct LoadInfo *load, struct Node *n)
{
	char propid[100], full_propid[300];
	U_LONG id_row, id_col, pi, pi_lc;

	while(true)
	{
		if(!GetNextSGFChar(load, true, E_VARIATION_NESTING))
			return false;

		switch(*load->current)
		{
			case '(':	/* ( ) ; indicate node end */
			case ')':
			case ';':	return true;
			case ']':	PrintError(E_ILLEGAL_OUTSIDE_CHAR, load->sgfc, load->cur_row, load->cur_col, true, load->current);
						NextChar(load);
						break;
			case '[':	PrintError(E_VALUES_WITHOUT_ID, load->sgfc, load->cur_row, load->cur_col);
						if(!SkipValues(load, true))
							return false;
						break;

			default:	/* isalpha */
				id_row = load->cur_row;
				id_col = load->cur_col;
				pi = 0;		/* counter for propid */
				pi_lc = 0;	/* counter for lowercase propid */

				if(load->lowercase)
				{
					U_LONG lc = load->lowercase >= 200 ? 199 : load->lowercase;
					strncpy(full_propid, load->current - load->lowercase, lc);
					pi_lc = lc;
					id_col -= load->lowercase;
				}

				while(!SGF_EOF)
				{
					if(islower((unsigned char)*load->current))
					{
						if(pi_lc < 200)
						{
							full_propid[pi + pi_lc] = *load->current;
							pi_lc++;
						}
					}
					else if(isupper((unsigned char)*load->current))
					{
						if(pi < 100)						/* max. 100 uc chars */
						{
							full_propid[pi+pi_lc] = *load->current;
							propid[pi++] = *load->current;
						}
					}
					else									/* end of PropID? */
					{
						propid[pi >= 100 ? 99 : pi] = 0;
						full_propid[pi+pi_lc >= 300 ? 299 : pi+pi_lc] = 0;

						if(pi >= 100)
							break;

						if(!GetNextSGFChar(load, true, E_UNEXPECTED_EOF))
							return false;

						if(*load->current != '[')
						{
							PrintError(E_NO_PROP_VALUES, load->sgfc, id_row, id_col, full_propid);
							break;
						}

						if(pi > 2)
							PrintError(WS_LONG_PROPID, load->sgfc, load->cur_row, load->cur_col, full_propid);

						int i = 1;
						for(; sgf_token[i].id; i++)
							if(!strcmp(propid, sgf_token[i].id))
								break;

						if(!sgf_token[i].id)	/* EOF sgf_token */
						{
							if(!load->sgfc->options->keep_unknown_props)
							{
								PrintError(WS_UNKNOWN_PROPERTY, load->sgfc, id_row, id_col, full_propid, "deleted");
								if(!SkipValues(load, true))
									return false;
								break;
							}
							PrintError(WS_UNKNOWN_PROPERTY, load->sgfc, id_row, id_col, full_propid, "found");
							i = TKN_UNKNOWN;
						}

						if(load->sgfc->options->delete_property[i])
						{
							PrintError(W_PROPERTY_DELETED, load->sgfc, id_row, id_col, "", full_propid);
							if(!SkipValues(load, true))
								return false;
							break;
						}

						if(!NewProperty(load, n, (token)i, id_row, id_col, full_propid))
							return false;
						break;
					}
					NextChar(load);
				}

				if(SGF_EOF)
				{
					PrintError(E_UNEXPECTED_EOF, load->sgfc, load->cur_row, load->cur_col);
					return false;
				}

				if(pi >= 100)
				{
					PrintError(E_PROPID_TOO_LONG, load->sgfc, id_row, id_col, full_propid);
					if(!SkipValues(load, true))
						return false;
				}
				break;
		}
	}
}


/**************************************************************************
*** Function:	NewNodeWithProperties
***				Small helper function for creating node and parsing properties
*** Parameters: load 	... pointer to LoadInfo structure
***				parent	... parent node
*** Returns:	pointer to Node or NULL
**************************************************************************/

static struct Node *NewNodeWithProperties(struct LoadInfo *load, struct Node *parent)
{
	struct Node *n = NewNode(load->sgfc, parent, load->cur_row, load->cur_col, false);

	if(!MakeProperties(load, n))
		return NULL;

	return n;
}


/**************************************************************************
*** Function:	BuildSGFTree
***				Recursive function to build up the sgf tree structure
*** Parameters: load ... pointer to LoadInfo structure
***				r	 ... tree root
***				missing_semicolon ... whether missing semicolon is known/reported already
*** Returns:	true or false on success/error
**************************************************************************/

static bool BuildSGFTree(struct LoadInfo *load, struct Node *r, bool missing_semicolon)
{
	int end_tree = 0, empty = 1;

	while(GetNextSGFChar(load, true, E_VARIATION_NESTING))
	{
		switch(*load->current)
		{
			case ';':	if(end_tree)
						{
							PrintError(E_NODE_OUTSIDE_VAR, load->sgfc, load->cur_row, load->cur_col);
							if(!BuildSGFTree(load, r, false))
								return false;
							end_tree = 1;
						}
						else
						{
							empty = 0;
							NextChar(load);
							r = NewNodeWithProperties(load, r);
							if(!r)
								return false;
						}
						break;
			case '(':	if(empty)
						{
							if(!missing_semicolon)
								PrintError(E_VARIATION_START, load->sgfc, load->cur_row, load->cur_col);
							NextChar(load);
						}
						else
						{
							NextChar(load);
							if(!BuildSGFTree(load, r, false))
								return false;
							end_tree = 1;
						}
						break;
			case ')':	if(empty)
							PrintError(E_EMPTY_VARIATION, load->sgfc, load->cur_row, load->cur_col);
						NextChar(load);
						return true;

			default:	if(empty)		/* assume there's a missing ';' */
						{
							if(!missing_semicolon)
								PrintError(E_MISSING_NODE_START, load->sgfc,
				   						   load->cur_row, load->cur_col - load->lowercase);
							empty = 0;
							r = NewNodeWithProperties(load, r);
							if(!r)
								return false;
						}
						else
						{
							PrintError(E_ILLEGAL_OUTSIDE_CHARS, load->sgfc,
				  					   load->cur_row, load->cur_col - load->lowercase,
									   true, load->current - load->lowercase, load->lowercase);
							NextChar(load);
						}
						break;
		}
	}

	return false;
}


/**************************************************************************
*** Function:	FindStart
***				sets load->current to '(' of start mark '(;'
*** Parameters: load	   ... pointer to LoadInfo structure
***				first_time ... search for the first time?
***							   (true -> if search fails -> fatal error)
*** Returns:	0 ... ok / 1 ... missing ';'  / -1 ... fatal error
**************************************************************************/

static int FindStart(struct LoadInfo *load, bool first_time)
{
	int warn = 0, o, c;
	const char *tmp;

	while(!SGF_EOF)
	{
		/* search for '[' (lc) (lc) ']' */
		if((load->current + 4 <= load->b_end) &&
		  (*load->current == '['))
			if(islower((unsigned char)*(load->current+1)) && islower((unsigned char)*(load->current+2)) &&
			  (*(load->current+3) == ']'))
			{
				if(!warn)		/* print warning only once */
				{
					PrintError(W_SGF_IN_HEADER, load->sgfc, load->cur_row, load->cur_col);
					warn = 1;
				}

				if(!first_time)
					PrintError(E_ILLEGAL_OUTSIDE_CHARS, load->sgfc, load->cur_row, load->cur_col, true, load->current, 4UL);

				load->current += 4;	/* skip '[aa]' */
				continue;
			}

		if(*load->current == '(')	/* test for start mark '(;' */
		{
			tmp = load->current + 1;
			while((tmp < load->b_end) && isspace((unsigned char)*tmp))
				tmp++;

			if(tmp == load->b_end)
				break;

			if(*tmp == ';')
				return 0;

			o = c = 0;

			if(load->sgfc->options->find_start == OPTION_FINDSTART_SEARCH)
			{		/* found a '(' but no ';' -> might be a missing ';' */
				tmp = load->current + 1;
				while((tmp != load->b_end) && *tmp != ')' && *tmp != '(')
				{
					if(*tmp == '[')		o++;
					if(*tmp == ']')		c++;
					tmp++;
				}
			}

			if((load->sgfc->options->find_start == OPTION_FINDSTART_BRACKET) ||
			   ((o >= 2) && (o >= c) && (o-c <= 1)))
			{
				PrintError(E_MISSING_SEMICOLON, load->sgfc, load->cur_row, load->cur_col);
				return 2;
			}
		}
		else
			if(!first_time && !isspace((unsigned char)*load->current))
				PrintError(E_ILLEGAL_OUTSIDE_CHAR, load->sgfc, load->cur_row, load->cur_col, true, load->current);

		NextChar(load);
	}

	if(first_time)
	{
		PrintError(FE_NO_SGFDATA, load->sgfc);
		return -1;
	}

	return 0;
}


/**************************************************************************
*** Function:	LoadSGF
***				Loads a SGF file into the memory and inits all
***				necessary information in SGFInfo structure
***
***             Note that some property values might actually be parsed
***             the wrong way (e.g. compose type stone values in GM[] != 1)
***             as this function doesn't keep track of such context across
***             properties, nodes, or trees. Only after ParseSGF() the
***             SGF game tree and its properties will be in proper shape.
***
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

	if(fseek(file, 0, SEEK_END) == -1)
		goto load_error;
	size = ftell(file);
	if(size == -1L || size == LONG_MAX) /* Linux may return LONG_MAX in some cases :o( */
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
*** Parameters: load ... pointer to LoadInfo structure
*** Returns:	true on success, false on fatal error
**************************************************************************/

bool LoadSGFFromFileBuffer(struct SGFInfo *sgfc)
{
	struct LoadInfo load;
	char *decode_buffer = NULL;

	load.sgfc = sgfc;
	load.buffer = sgfc->buffer;
	load.b_end = sgfc->b_end;
	load.current = sgfc->buffer;
	load.cur_row = 1;
	load.cur_col = 1;
	load.lowercase = 0;
	load.is_utf8 = false;

	if(sgfc->options->encoding == OPTION_ENCODING_EVERYTHING)
	{
		if(!(decode_buffer = DecodeSGFBuffer(sgfc, &load.b_end, &sgfc->global_encoding_name)))
			return false;
		load.buffer = decode_buffer;
		load.current = decode_buffer;
		load.is_utf8 = true;
	}

	int miss = FindStart(&load, true);	/* skip junk in front of '(;' */
	if(miss == -1)
	{
		free(decode_buffer);
		return false;
	}

	sgfc->start = load.current;

	while(load.current < load.b_end)
	{
		if(!miss)
			NextChar(&load);				/* skip '(' */
		if(!BuildSGFTree(&load, NULL, miss==2))
			break;
		miss = FindStart(&load, false);		/* skip junk in front of '(;' */
	}

	PrintError(E_NO_ERROR, sgfc);		/* flush accumulated messages */
	free(decode_buffer);
	return true;
}
