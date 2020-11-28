/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 save.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "all.h"
#include "protos.h"


#define MAX_LINELEN		58
#define MAXTEXT_LINELEN 70

/* This is used when options->node_linebreaks is set, so we are
 * attempting to put linebreaks at the end of nodes.  A value near
 * MAX_LINELEN is desirable so that we don't make lines much shorter
 * or longer than we get without this option.  A value of 60 works out
 * well.  If nodes are of the form ";B[xx]", we'll get exactly 10 of
 * them per line, just like we would get without this option set. */
#define	MAX_PREDICTED_LINELEN	60

#define saveputc(s,c) { if(!WriteChar((s), (c), false))	return false;	}

#define CheckLineLen(s) { if((s)->_save_c->linelen > MAX_LINELEN) \
						{ saveputc(s, '\n')	} }


/* internal data for save.c functions. */
/* Used instead of local static variables */
struct SaveC_internal {
	int linelen;		/* used for line breaking algorithm */
	int chars_in_node;
	int eol_in_node;
	bool gi_written;	/* used by WriteProperty for newlines after gameinfo properties */
};


/**************************************************************************
*** Function:	SetupSaveC_internal
***				Allocate and initialize internal data structure local to save.c
*** Parameters: -
*** Returns:	pointer to internal structure
**************************************************************************/

struct SaveC_internal *SetupSaveC_internal(void)
{
	struct SaveC_internal *savec = SaveCalloc(sizeof(struct SaveC_internal), "static save.c struct");
	return savec;
}


/**************************************************************************
*** Function:	SetupSaveFileIO
***				Allocate and initialize SaveFileHandler for regular file access
*** Parameters: -
*** Returns:	pointer to SafeFileHandler
**************************************************************************/

static int SaveFileIO_open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	sfh->fh.file = fopen(path, mode);
	return !!sfh->fh.file;
}

static int SaveFileIO_close(struct SaveFileHandler *sfh, U_LONG error)
{
	return fclose(sfh->fh.file);
}

static int SaveFileIO_putc(struct SaveFileHandler *sfh, int c)
{
	return fputc(c, sfh->fh.file);
}

struct SaveFileHandler *SetupSaveFileIO(void)
{
	struct SaveFileHandler *sfh = SaveMalloc(sizeof(struct SaveFileHandler), "file handler");
	sfh->open = SaveFileIO_open;
	sfh->close = SaveFileIO_close;
	sfh->putc = SaveFileIO_putc;
	sfh->fh.file = NULL;
	return sfh;
}


/**************************************************************************
*** Function:	SaveBufferIO_open
***				Initializes SaveBuffer structure for saving to memory
***             Allocates 5000 bytes as initial value
*** Parameters: sfh ... pointer to SaveFileHandler
***				path, mode ... dummy
*** Returns:	true on success, false on error (out of memory)
**************************************************************************/

static int SaveBufferIO_open(struct SaveFileHandler *sfh, const char *path, const char *mode)
{
	/* Start with ~5kb buffer which suffices in many cases */
	sfh->fh.memh.buffer = (char *)malloc((size_t)5000);
	if(!sfh->fh.memh.buffer)
		return false;
	sfh->fh.memh.buffer_size = 5000;
	sfh->fh.memh.pos = sfh->fh.memh.buffer;
	return true;
}


/**************************************************************************
*** Function:	SaveBufferIO_close
***				Frees buffer inside SaveBuffer structure
*** Parameters: sfh ... pointer to SaveFileHandler
***				error ... SGFC error code or E_NO_ERROR
***                       (useful for functions that replace this one)
*** Returns:	true
**************************************************************************/

int SaveBufferIO_close(struct SaveFileHandler *sfh, U_LONG error)
{
	free(sfh->fh.memh.buffer);
	sfh->fh.memh.buffer = NULL;
	sfh->fh.memh.pos = NULL;
	sfh->fh.memh.buffer_size = 0;
	return true;
}


/**************************************************************************
*** Function:	SaveBufferIO_putc
***				Writes char to buffer, allocates more memory if current
***				buffer is too small to hold next char.
*** Parameters: sfh ... pointer to SaveFileHandler
***				c   ... char to write
*** Returns:	char written or EOF in case of error
**************************************************************************/

static int SaveBufferIO_putc(struct SaveFileHandler *sfh, int c)
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
	return c;
}


/**************************************************************************
*** Function:	SetupSaveBufferIO
***				Allocates and initializes SaveFileHandler for BufferIO
*** Parameters: close ... custom close() function or NULL
*** Returns:	pointer to SaveFileHandler
**************************************************************************/

struct SaveFileHandler *SetupSaveBufferIO(int (*close)(struct SaveFileHandler *, U_LONG))
{
	struct SaveFileHandler *sfh = SaveMalloc(sizeof(struct SaveFileHandler), "memory file handler");
	sfh->open = SaveBufferIO_open;
	sfh->putc = SaveBufferIO_putc;
	if(close)
		sfh->close = close;
	else
		sfh->close = SaveBufferIO_close;
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
*** Returns:	true or false
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
			return false;
	}
	else
	{
		sgfc->_save_c->eol_in_node = 1;
		sgfc->_save_c->linelen = 0;

#if EOLCHAR
		if((*sgfc->sfh->putc)(sgfc->sfh, EOLCHAR) == EOF)
			return false;
#else
		if((*sgfc->sfh->putc)(sgfc->sfh, '\r') == EOF)		/* MSDOS EndOfLine */
			return false;
		if((*sgfc->sfh->putc)(sgfc->sfh, '\n') == EOF)
			return false;
#endif
	}

	return true;
}


/**************************************************************************
*** Function:	WritePropValue
***				Value into the given file
***				does necessary conversions
*** Parameters: sgfc   ... pointer to SGFInfo
***				v	   ... pointer to value
***				second ... true: write value2, false:write value1
***				flags  ... property flags
*** Returns:	true or false
**************************************************************************/

static int WritePropValue(struct SGFInfo *sgfc, const char *v, bool second, U_SHORT flags)
{
	U_SHORT fl;

	if(!v)	return true;

	if(second)
		saveputc(sgfc, ':')

	fl = sgfc->options->soft_linebreaks && (flags & SPLIT_SAVE);

	while(*v)
	{
		if(*v == '\\' || *v == ']' || (flags & PVT_COMPOSE && *v == ':'))
			saveputc(sgfc, '\\');

		if(!WriteChar(sgfc, *v, flags & PVT_SIMPLE))
			return false;

		if(fl && (sgfc->_save_c->linelen > MAXTEXT_LINELEN)) /* soft linebreak */
		{
			saveputc(sgfc, '\\')	/* insert soft linebreak */
			saveputc(sgfc, '\n')
		}

		v++;
	}

	return true;
}

/**************************************************************************
*** Function:	WriteProperty
***				writes ID & value into the given file
***				does necessary conversions
*** Parameters: sgfc  ... pointer to SGFInfo
***				info ... game-tree info
***				prop ... pointer to property
*** Returns:	true or false
**************************************************************************/

static int WriteProperty(struct SGFInfo *sgfc, struct TreeInfo *info, struct Property *prop)
{
	struct PropValue *v;
	char *p;
	bool do_tt;

	if(prop->flags & TYPE_GINFO)
	{
		if(!sgfc->_save_c->gi_written)
		{
			saveputc(sgfc, '\n')
			saveputc(sgfc, '\n')
		}
		sgfc->_save_c->gi_written = true;
	}
	else
		sgfc->_save_c->gi_written = false;


	p = prop->idstr;			/* write property ID */
	while(*p)
	{
		/* idstr is original from file -> may contain lowercase too */
		if(isupper(*p))
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

		if(do_tt && !v->value_len)
			WritePropValue(sgfc, "tt", false, prop->flags);
		else
		{
			WritePropValue(sgfc, v->value, false, prop->flags);
			WritePropValue(sgfc, v->value2, true, prop->flags);
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

	return true;
}


/**************************************************************************
*** Function:	WriteNode
***				writes the node char ';' calls WriteProperty for all props
*** Parameters: sgfc ... pointer to SGFInfo
***				info ... pointer current TreeInfo
***				n	 ... node to write
*** Returns:	true or false
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
		if((sgf_token[p->id].flags & PVT_CPLIST) && !sgfc->options->expand_cpl &&
		   (info->GM == 1))
			CompressPointList(sgfc, p);

		if(!WriteProperty(sgfc, info, p))
			return false;

		p = p->next;
	}

	if(sgfc->options->node_linebreaks &&
	   ((sgfc->_save_c->eol_in_node && sgfc->_save_c->linelen > 0) ||
		(!sgfc->_save_c->eol_in_node &&
		  sgfc->_save_c->linelen > MAX_PREDICTED_LINELEN - sgfc->_save_c->chars_in_node)))
		saveputc(sgfc, '\n')

	return true;
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

	NewPropValue(sgfc, r, TKN_FF, "4", NULL, true);

	if(sgfc->options->encoding != OPTION_ENCODING_NONE)
		NewPropValue(sgfc, r, TKN_CA, "UTF-8", NULL, true);

	if(sgfc->options->add_sgfc_ap_property)
		NewPropValue(sgfc, r, TKN_AP, "SGFC", "1.18", true);

	if(info->GM == 1)
	{
		NewPropValue(sgfc, r, TKN_GM, "1", NULL, true);
		if(info->bwidth == 19 && info->bheight == 19)
			NewPropValue(sgfc, r, TKN_SZ, "19", NULL, true);
	}
}


/**************************************************************************
*** Function:	WriteTree
***				recursive function which writes a complete SGF tree
*** Parameters: sgfc ... pointer to SGFInfo
***				info 	 ... TreeInfo
***				n		 ... root node of tree
***				newlines ... number of nl to print
*** Returns:	true: success / false error
**************************************************************************/

static int WriteTree(struct SGFInfo *sgfc, struct TreeInfo *info,
					 struct Node *n, int newlines)
{
	if(newlines && sgfc->_save_c->linelen > 0)
		saveputc(sgfc, '\n')

	SetRootProps(sgfc, info, n);

	saveputc(sgfc, '(')
	if(!WriteNode(sgfc, info, n))
		return false;

	n = n->child;

	while(n)
	{
		if(n->sibling)
		{
			while(n)					/* write child + variations */
			{
				if(!WriteTree(sgfc, info, n, 1))
					return false;
				n = n->sibling;
			}
		}
		else
		{
			if(!WriteNode(sgfc, info, n))	/* write child */
				return false;
			n = n->child;
		}
	}

	saveputc(sgfc, ')')

	if(newlines != 1)
		saveputc(sgfc, '\n')

	return true;
}


/**************************************************************************
*** Function:	SaveSGF
***				writes the complete SGF tree to a file
*** Parameters: sgfc      ... pointer to SGFInfo structure
***				base_name ... filename/path of destination file
*** Returns:	true on success, false on error while writing file(s)
**************************************************************************/

bool SaveSGF(struct SGFInfo *sgfc, const char *base_name)
{
	struct Node *n;
	struct TreeInfo *info;
	const char *c;
	int nl = 0, i = 1;
	size_t name_buffer_size = strlen(base_name) + 14; /* +14 == "_99999999.sgf" + \0 */

	char *name = SaveMalloc(name_buffer_size, "filename buffer");
	if(sgfc->options->split_file)
		snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);
	else
		strcpy(name, base_name);

	if(!(*sgfc->sfh->open)(sgfc->sfh, name, "wb"))
	{
		PrintError(FE_DEST_FILE_OPEN, sgfc, name);
		free(name);
		return false;
	}

	if(sgfc->options->keep_head)
	{
		for(c = sgfc->buffer; c < sgfc->start; c++)
			if((*sgfc->sfh->putc)(sgfc->sfh, *c) == EOF)
				goto write_error;
		if((*sgfc->sfh->putc)(sgfc->sfh, '\n') == EOF)
			goto write_error;
	}

	sgfc->_save_c->linelen = 0;
	sgfc->_save_c->chars_in_node = 0;
	sgfc->_save_c->eol_in_node = 0;

	n = sgfc->root;
	info = sgfc->tree;

	while(n)
	{
		if(!WriteTree(sgfc, info, n, nl))
			goto write_error;

		nl = 2;
		n = n->sibling;
		info = info->next;

		if(sgfc->options->split_file && n)
		{
			(*sgfc->sfh->close)(sgfc->sfh, E_NO_ERROR);
			i++;
			snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);

			if(!(*sgfc->sfh->open)(sgfc->sfh, name, "wb"))
			{
				PrintError(FE_DEST_FILE_OPEN, sgfc, name);
				free(name);
				return false;
			}
		}
	}

	(*sgfc->sfh->close)(sgfc->sfh, E_NO_ERROR);
	free(name);
	return true;

write_error:
	(*sgfc->sfh->close)(sgfc->sfh, FE_DEST_FILE_WRITE);
	PrintError(FE_DEST_FILE_WRITE, sgfc, name);
	free(name);
	return false;
}
