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

#define CheckLineLen(s) { if((s)->_save_c->linelen > MAX_LINELEN) \
						{ saveputc(s, '\n')	} }


/* internal data for save.c functions. */
/* Used instead of local static variables */
struct Save_C_internal {
	int linelen;	/* used for line breaking algorithm */
	int chars_in_node;
	int eol_in_node;
	int gi_written; /* used by WriteProperty for newlines after gameinfo properties */
};


/**************************************************************************
*** Function:	Setup_Save_C_internal
***				Allocate and initialize internal data structure local to save.c
*** Parameters: -
*** Returns:	pointer to internal structure
**************************************************************************/

struct Save_C_internal *Setup_Save_C_internal(void)
{
	struct Save_C_internal *savec;
	SaveMalloc(struct Save_C_internal *, savec, sizeof(struct Save_C_internal), "static save.c struct")
	savec->gi_written = FALSE;
	return savec;
}


/**************************************************************************
*** Function:	Setup_SaveFileIO
***				Allocate and initialize SaveFileHandler for regular file access
*** Parameters: -
*** Returns:	pointer to SafeFileHandler
**************************************************************************/

static int SaveFile_FileIO_Open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	sfh->fh.file = fopen(path, mode);
	return(!!sfh->fh.file);
}

static int SaveFile_FileIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	return(fclose(sfh->fh.file));
}

static int SaveFile_FileIO_Putc(struct SaveFileHandler *sfh, int c)
{
	return(fputc(c, sfh->fh.file));
}

struct SaveFileHandler *Setup_SaveFileIO(void)
{
	struct SaveFileHandler *sfh;
	SaveMalloc(struct SaveFileHandler *, sfh, sizeof(struct SaveFileHandler), "file handler")
	sfh->open = SaveFile_FileIO_Open;
	sfh->close = SaveFile_FileIO_Close;
	sfh->putc = SaveFile_FileIO_Putc;
	sfh->fh.file = NULL;
	return sfh;
}


/**************************************************************************
*** Function:	SaveFile_BufferIO_Open
***				Initializes SaveBuffer structure for saving to memory
***             Allocates 5000 bytes as initial value
*** Parameters: sfh ... pointer to SaveFileHandler
***				path, mode ... dummy
*** Returns:	TRUE on success, FALSE on error (out of memory)
**************************************************************************/

static int SaveFile_BufferIO_Open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	/* Start with ~5kb buffer which suffices in many cases */
	sfh->fh.memh.buffer = (char *)malloc((size_t)5000);
	if(!sfh->fh.memh.buffer)
		return(FALSE);
	sfh->fh.memh.buffer_size = 5000;
	sfh->fh.memh.pos = sfh->fh.memh.buffer;
	return(TRUE);
}


/**************************************************************************
*** Function:	SaveFile_BufferIO_Close
***				Frees buffer inside SaveBuffer structure
*** Parameters: sfh ... pointer to SaveFileHandler
***				error ... SGFC error code or E_NO_ERROR
***                       (useful for functions that replace this one)
*** Returns:	TRUE
**************************************************************************/

int SaveFile_BufferIO_Close(struct SaveFileHandler *sfh, U_LONG error)
{
	free(sfh->fh.memh.buffer);
	sfh->fh.memh.buffer = NULL;
	sfh->fh.memh.pos = NULL;
	sfh->fh.memh.buffer_size = 0;
	return(TRUE);
}


/**************************************************************************
*** Function:	SaveFile_BufferIO_Putc
***				Writes char to buffer, allocates more memory if current
***				buffer is too small to hold next char.
*** Parameters: sfh ... pointer to SaveFileHandler
***				c   ... char to write
*** Returns:	char written or EOF in case of error
**************************************************************************/

static int SaveFile_BufferIO_Putc(struct SaveFileHandler *sfh, int c)
{
	/* -1 so that we can always null-terminate buffer in close() function */
	if (sfh->fh.memh.pos == sfh->fh.memh.buffer + sfh->fh.memh.buffer_size - 1)
	{
		/* size*2 ... typical strategy used by ArrayList structures */
		char *new_buffer = (char *)malloc(sfh->fh.memh.buffer_size*2);
		if (!new_buffer)
			return EOF;
		memcpy(new_buffer, sfh->fh.memh.buffer, sfh->fh.memh.buffer_size);
		free(sfh->fh.memh.buffer);
		sfh->fh.memh.buffer = new_buffer;
		sfh->fh.memh.pos = new_buffer + sfh->fh.memh.buffer_size;
		sfh->fh.memh.buffer_size *= 2;
	}

	*sfh->fh.memh.pos++ = (char)c;
	return(c);
}


/**************************************************************************
*** Function:	Setup_SaveBufferIO
***				Allocates and initializes SaveFileHandler for BufferIO
*** Parameters: close ... custom close() function or NULL
*** Returns:	pointer to SaveFileHandler
**************************************************************************/

struct SaveFileHandler *Setup_SaveBufferIO(int (*close)(struct SaveFileHandler *, U_LONG))
{
	struct SaveFileHandler *sfh;
	SaveMalloc(struct SaveFileHandler *, sfh, sizeof(struct SaveFileHandler), "memory file handler")
	sfh->open = SaveFile_BufferIO_Open;
	sfh->putc = SaveFile_BufferIO_Putc;
	if(close)
		sfh->close = close;
	else
		sfh->close = SaveFile_BufferIO_Close;
	sfh->fh.memh.buffer = NULL;
	sfh->fh.memh.pos = NULL;
	sfh->fh.memh.buffer_size = 0;
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
	sgfc->_save_c->chars_in_node++;

	if(spc && isspace(c) && (sgfc->_save_c->linelen >= MAXTEXT_LINELEN))
		c = '\n';

	if(c != '\n')
	{
		sgfc->_save_c->linelen++;

		if((*sgfc->sfh->putc)(sgfc->sfh, c) == EOF)
			return(FALSE);
	}
	else
	{
		sgfc->_save_c->eol_in_node = 1;
		sgfc->_save_c->linelen = 0;

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
*** Parameters: sgfc   ... pointer to SGFInfo
***				v	   ... pointer to value
***				second ... TRUE: write value2, FALSE:write value1
***				flags  ... property flags
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

		if(fl && (sgfc->_save_c->linelen > MAXTEXT_LINELEN)) /* soft linebreak */
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
*** Parameters: sgfc  ... pointer to SGFInfo
***				info ... game-tree info
***				prop ... pointer to property
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WriteProperty(struct SGFInfo *sgfc, struct TreeInfo *info, struct Property *prop)
{
	struct PropValue *v;
	char *p;
	int do_tt;

	if(prop->flags & TYPE_GINFO)
	{
		if(!sgfc->_save_c->gi_written)
		{
			saveputc(sgfc, '\n')
			saveputc(sgfc, '\n')
		}
		sgfc->_save_c->gi_written = TRUE;
	}
	else
		sgfc->_save_c->gi_written = FALSE;


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
*** Parameters: sgfc ... pointer to SGFInfo
***				info ... pointer current TreeInfo
***				n	 ... node to write
*** Returns:	TRUE or FALSE
**************************************************************************/

static int WriteNode(struct SGFInfo *sgfc, struct TreeInfo *info, struct Node *n)
{
	struct Property *p;
	sgfc->_save_c->chars_in_node = 0;
	sgfc->_save_c->eol_in_node = 0;
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
	   ((sgfc->_save_c->eol_in_node && sgfc->_save_c->linelen > 0) ||
		(!sgfc->_save_c->eol_in_node &&
		  sgfc->_save_c->linelen > MAX_PREDICTED_LINELEN - sgfc->_save_c->chars_in_node)))
		saveputc(sgfc, '\n')

	return(TRUE);
}


/**************************************************************************
*** Function:	SetRootProps
***				Sets new root properties for the game tree
*** Parameters: sgfc ... pointer to SGFInfo
***				info ... TreeInfo
***				r	 ... root node of tree
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
*** Parameters: sgfc ... pointer to SGFInfo
***				info 	 ... TreeInfo
***				n		 ... root node of tree
***				newlines ... number of nl to print
*** Returns:	TRUE: success / FALSE error
**************************************************************************/

static int WriteTree(struct SGFInfo *sgfc, struct TreeInfo *info,
					 struct Node *n, int newlines)
{
	if(newlines && sgfc->_save_c->linelen > 0)
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
*** Parameters: sgfc      ... pointer to SGFInfo structure
***				base_name ... filename/path of destination file
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

	sgfc->_save_c->linelen = 0;
	sgfc->_save_c->chars_in_node = 0;
	sgfc->_save_c->eol_in_node = 0;

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
