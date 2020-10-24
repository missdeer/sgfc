/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 save.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "all.h"
#include "protos.h"


static int save_linelen;
static int save_chars_in_node;
static int save_eol_in_node;

#define MAX_LINELEN		58
#define MAXTEXT_LINELEN 70

/* This is used when options->nodelinebreaks is set, so we are
 * attempting to put linebreaks at the end of nodes.  A value near
 * MAX_LINELEN is desirable so that we don't make lines much shorter
 * or longer than we get without this option.  A value of 60 works out
 * well.  If nodes are of the form ";B[xx]", we'll get exactly 10 of
 * them per line, just like we would get without this option set. */
#define	MAX_PREDICTED_LINELEN	60

#define saveputc(s,c) { if(!WriteChar((s), (c), FALSE))	return(FALSE);	}

#define CheckLineLen(s) { if(save_linelen > MAX_LINELEN) \
						{ saveputc(s, '\n')	} }


static int SaveFile_FileIO_Open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	sfh->fh = fopen(path, mode);
	return(!!sfh->fh);
}

static int SaveFile_FileIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	return fclose(sfh->fh);
}

static int SaveFile_FileIO_Putc(struct SaveFileHandler *sfh, int c)
{
	return fputc(c, sfh->fh);
}


static int SaveFile_BufferIO_Open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	/* Start with ~5kb buffer which suffices in many cases */
	sfh->buffer.buffer = (char *)malloc((size_t)5000);
	if(!sfh->buffer.buffer)
		return(FALSE);
	sfh->buffer.buffer_size = 5000;
	sfh->buffer.pos = sfh->buffer.buffer;
	return(TRUE);
}

int SaveFile_BufferIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	free(sfh->buffer.buffer);
	sfh->buffer.buffer = NULL;
	sfh->buffer.pos = NULL;
	sfh->buffer.buffer_size = 0;
	return(TRUE);
}

static int SaveFile_BufferIO_Putc(struct SaveFileHandler *sfh, int c)
{
	/* -1 so that we can always null-terminate buffer in close() function */
	if (sfh->buffer.pos == sfh->buffer.buffer + sfh->buffer.buffer_size - 1)
	{
		char *new_buffer = (char *)malloc(sfh->buffer.buffer_size*2);
		if (!new_buffer)
			return EOF;
		memcpy(new_buffer, sfh->buffer.buffer, sfh->buffer.buffer_size);
		free(sfh->buffer.buffer);
		sfh->buffer.buffer = new_buffer;
		sfh->buffer.pos = new_buffer + sfh->buffer.buffer_size;
		sfh->buffer.buffer_size *= 2;
	}

	*sfh->buffer.pos++ = (char)c;
	return(c);
}

struct SaveFileHandler save_file_io = {
		SaveFile_FileIO_Open,
		SaveFile_FileIO_Close,
		SaveFile_FileIO_Putc,
		NULL
};

struct SaveFileHandler *init_save_buffer_io(int (*close)(struct SaveFileHandler *, U_LONG))
{
	struct SaveFileHandler *sfh;
	SaveMalloc(struct SaveFileHandler *, sfh, sizeof(struct SaveFileHandler), "memory file handler")
	sfh->open = SaveFile_BufferIO_Open;
	sfh->putc = SaveFile_BufferIO_Putc;
	if(close)
		sfh->close = close;
	else
		sfh->close = SaveFile_BufferIO_Close;
	sfh->buffer.buffer = NULL;
	sfh->buffer.pos = NULL;
	sfh->buffer.buffer_size = 0;
	return sfh;
}


/**************************************************************************
*** Function:	WriteChar
***				Writes char to file, modifies save_linelen
***				transforms EndOfLine-Char if necessary
*** Parameters: sgfc  ... pointer to SGFInfo
***				c     ... char to be written
***				spc   ... convert spaces to EOL if line is too long
***                       (used only on PVT_SIMPLE property values)
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WriteChar(struct SGFInfo *sgfc, char c, U_SHORT spc)
{
	save_chars_in_node++;

	if(spc && isspace(c) && (save_linelen >= MAXTEXT_LINELEN))
		c = '\n';

	if(c != '\n')
	{
		save_linelen++;

		if((*sgfc->sfh->putc)(sgfc->sfh, c) == EOF)
			return(FALSE);
	}
	else
	{
		save_eol_in_node = 1;
		save_linelen = 0;

#if EOLCHAR
		if((*sgfc->sfh->putc)(sgfc->sfh, EOLCHAR) == EOF)
			return(FALSE);
#else
		if((*sgfc->sfh->putc)(sgfc->sfh, '\r') == EOF)		/* MSDOS EndOfLine */
			return(FALSE);
		if((*sgfc->sfh->putc)(sgfc->sfh, '\n') == EOF)
			return(FALSE);
#endif
	}

	return(TRUE);
}


/**************************************************************************
*** Function:	WritePropValue
***				Value into the given file
***				does necessary conversions
*** Parameters: v ... pointer to value
***				second ... TRUE: write value2, FALSE:write value1
***				flags ... property flags
***				sfile .. file handle
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WritePropValue(struct SGFInfo *sgfc, const char *v, int second, U_SHORT flags)
{
	U_SHORT fl;

	if(!v)	return(TRUE);

	if(second)
		saveputc(sgfc, ':')

	fl = sgfc->options->softlinebreaks && (flags & SPLIT_SAVE);

	while(*v)
	{
		if(!WriteChar(sgfc, *v, flags & PVT_SIMPLE))
			return(FALSE);

		if(fl && (save_linelen > MAXTEXT_LINELEN))		/* soft linebreak */
		{
			if (*v == '\\' && *(v-1) != '\\')
								 /* if we have just written a single '\' then  */
				v--;					/* treat it as soft linebreak and set  */
			else						/* v back so that it is written again. */
				saveputc(sgfc, '\\')	/* else insert soft linebreak */
			saveputc(sgfc, '\n')
		}

		v++;
	}

	return(TRUE);
}

/**************************************************************************
*** Function:	WriteProperty
***				writes ID & value into the given file
***				does necessary conversions
*** Parameters: info ... game-tree info
***				prop ... pointer to property
***				sfile .. file handle
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WriteProperty(struct SGFInfo *sgfc, struct TreeInfo *info, struct Property *prop)
{
	static int gi_written = FALSE;

	struct PropValue *v;
	char *p;
	int do_tt;

	if(prop->flags & TYPE_GINFO)
	{
		if(!gi_written)
		{
			saveputc(sgfc, '\n')
			saveputc(sgfc, '\n')
		}
		gi_written = TRUE;
	}
	else
		gi_written = FALSE;


	p = prop->idstr;			/* write property ID */
	while(*p)
	{
		saveputc(sgfc, *p)
		p++;
	}

	do_tt = (info->GM == 1 && sgfc->options->pass_tt &&
			(info->bwidth <= 19) && (info->bheight <= 19) &&
			(prop->id == TKN_B || prop->id == TKN_W));

	v = prop->value;			/* write property value(s) */

	while(v)
	{
		saveputc(sgfc, '[')

		if(do_tt && !strlen(v->value))
			WritePropValue(sgfc, "tt", FALSE, prop->flags);
		else
		{
			WritePropValue(sgfc, v->value, FALSE, prop->flags);
			WritePropValue(sgfc, v->value2, TRUE, prop->flags);
		}
		saveputc(sgfc, ']')

		CheckLineLen(sgfc)
		v = v->next;
	}

	if(prop->flags & TYPE_GINFO)
	{
		saveputc(sgfc, '\n')
		if(prop->next)
		{
			if(!(prop->next->flags & TYPE_GINFO))
				saveputc(sgfc, '\n')
		}
		else
			saveputc(sgfc, '\n')
	}

	return(TRUE);
}


/**************************************************************************
*** Function:	WriteNode
***				writes the node char ';' calls WriteProperty for all props
*** Parameters: n ... node to write
***				sfile ... file handle
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WriteNode(struct SGFInfo *sgfc, struct TreeInfo *info, struct Node *n)
{
	struct Property *p;
	save_chars_in_node = 0;
	save_eol_in_node = 0;
	saveputc(sgfc, ';')

	p = n->prop;
	while(p)
	{
		if((sgf_token[p->id].flags & PVT_CPLIST) && !sgfc->options->expandcpl &&
		   (info->GM == 1))
			CompressPointList(sgfc, p);

		if(!WriteProperty(sgfc, info, p))
			return(FALSE);

		p = p->next;
	}

	if(sgfc->options->nodelinebreaks &&
	   ((save_eol_in_node && save_linelen > 0) ||
		(!save_eol_in_node && save_linelen > MAX_PREDICTED_LINELEN - save_chars_in_node)))
		saveputc(sgfc, '\n')

	return(TRUE);
}


/**************************************************************************
*** Function:	SetRootProps
***				Sets new root properties for the game tree
*** Parameters: info 	 ... TreeInfo
***				r		 ... root node of tree
*** Returns:	-
**************************************************************************/

static void SetRootProps(struct SGFInfo *sgfc, struct TreeInfo *info, struct Node *r)
{
	if(r->parent)	/* isn't REAL root node */
		return;

	New_PropValue(sgfc, r, TKN_FF, "4", NULL, TRUE);
	New_PropValue(sgfc, r, TKN_AP, "SGFC", "1.18", TRUE);

	if(info->GM == 1)			/* may be default value without property */
		New_PropValue(sgfc, r, TKN_GM, "1", NULL, TRUE);

								/* may be default value without property */
	if(info->bwidth == 19 && info->bheight == 19)
		New_PropValue(sgfc, r, TKN_SZ, "19", NULL, TRUE);
}


/**************************************************************************
*** Function:	WriteTree
***				recursive function which writes a complete SGF tree
*** Parameters: info 	 ... TreeInfo
***				n		 ... root node of tree
***				sfile	 ... file handle
***				newlines ... number of nl to print
*** Returns:	TRUE: success / FALSE error
**************************************************************************/

static int WriteTree(struct SGFInfo *sgfc, struct TreeInfo *info,
					 struct Node *n, int newlines)
{
	if(newlines && save_linelen > 0)
		saveputc(sgfc, '\n')

	SetRootProps(sgfc, info, n);

	saveputc(sgfc, '(')
	if(!WriteNode(sgfc, info, n))
		return(FALSE);

	n = n->child;

	while(n)
	{
		if(n->sibling)
		{
			while(n)					/* write child + variations */
			{
				if(!WriteTree(sgfc, info, n, 1))
					return(FALSE);
				n = n->sibling;
			}
		}
		else
		{
			if(!WriteNode(sgfc, info, n))	/* write child */
				return(FALSE);
			n = n->child;
		}
	}

	saveputc(sgfc, ')')

	if(newlines != 1)
		saveputc(sgfc, '\n')

	return(TRUE);
}


/**************************************************************************
*** Function:	SaveSGF
***				writes the complete SGF tree to a file
*** Parameters: sgf ... pointer to sgfinfo structure
*** Returns:	-
**************************************************************************/

void SaveSGF(struct SGFInfo *sgfc, char *base_name)
{
	struct Node *n;
	struct TreeInfo *info;
	char *c, *name;
	int nl = 0, i = 1;
	size_t name_buffer_size = strlen(base_name) + 14; /* +14 == "_99999999.sgf" + \0 */

	SaveMalloc(char *, name, name_buffer_size, "filename buffer")
	if(sgfc->options->split_file)
		snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);
	else
		strcpy(name, base_name);

	if(!(*sgfc->sfh->open)(sgfc->sfh, name, "wb"))
		PrintFatalError(FE_DEST_FILE_OPEN, sgfc, name);

	if(sgfc->options->keep_head)
	{
		*sgfc->start = '\n';

		for(c = sgfc->buffer; c <= sgfc->start; c++)
			if((*sgfc->sfh->putc)(sgfc->sfh, *c) == EOF)
			{
				(*sgfc->sfh->close)(sgfc->sfh, FE_DEST_FILE_WRITE);
				PrintFatalError(FE_DEST_FILE_WRITE, sgfc, name);
			}
	}

	save_linelen = 0;
	save_chars_in_node = 0;
	save_eol_in_node = 0;

	n = sgfc->root;
	info = sgfc->tree;

	while(n)
	{
		if(!WriteTree(sgfc, info, n, nl))
		{
			(*sgfc->sfh->close)(sgfc->sfh, FE_DEST_FILE_WRITE);
			PrintFatalError(FE_DEST_FILE_WRITE, sgfc, name);
		}

		nl = 2;
		n = n->sibling;
		info = info->next;

		if(sgfc->options->split_file && n)
		{
			(*sgfc->sfh->close)(sgfc->sfh, E_NO_ERROR);
			i++;
			snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);

			if(!(*sgfc->sfh->open)(sgfc->sfh, name, "wb"))
				PrintFatalError(FE_DEST_FILE_OPEN, sgfc, name);
		}
	}

	(*sgfc->sfh->close)(sgfc->sfh, E_NO_ERROR);
	free(name);
}
