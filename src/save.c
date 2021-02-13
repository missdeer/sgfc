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

#define CheckLineLen(s) { if((s)->linelen > MAX_LINELEN) \
						{ saveputc(s, '\n')	} }


/* internal data structure for save.c functions */
struct SaveInfo
{
	struct SGFInfo *sgfc;
	struct SaveFileHandler *sfh;
	
	int linelen;		/* used for line breaking algorithm */
	int chars_in_node;
	int eol_in_node;
	bool gi_written;	/* used by WriteProperty for newlines after gameinfo properties */
};


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
	FILE *file = sfh->fh.file;
	if(!file)
		return 0;
	sfh->fh.file = NULL;
	return fclose(file);
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

int SaveBufferIO_open(struct SaveFileHandler *sfh, const char *path, const char *mode)
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

struct SaveFileHandler *SetupSaveBufferIO(
	int (*open)(struct SaveFileHandler *, const char *, const char *),
	int (*close)(struct SaveFileHandler *, U_LONG))
{
	struct SaveFileHandler *sfh = SaveMalloc(sizeof(struct SaveFileHandler), "memory file handler");
	sfh->open = SaveBufferIO_open;
	sfh->putc = SaveBufferIO_putc;
	if(open)	sfh->open = open;
	else		sfh->open = SaveBufferIO_open;
	if(close)	sfh->close = close;
	else		sfh->close = SaveBufferIO_close;
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

static int WriteChar(struct SaveInfo *save, char c, U_SHORT spc)
{
	save->chars_in_node++;

	if(spc && isspace((unsigned char)c) && (save->linelen >= MAXTEXT_LINELEN))
		c = '\n';

	if(c != '\n')
	{
		save->linelen++;

		if((*save->sfh->putc)(save->sfh, c) == EOF)
			return false;
	}
	else
	{
		save->eol_in_node = 1;
		save->linelen = 0;

#if EOLCHAR
		if((*save->sfh->putc)(save->sfh, EOLCHAR) == EOF)
			return false;
#else
		if((*save->sfh->putc)(save->sfh, '\r') == EOF)		/* MSDOS EndOfLine */
			return false;
		if((*save->sfh->putc)(save->sfh, '\n') == EOF)
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

static int WritePropValue(struct SaveInfo *save, const char *v, bool second, U_SHORT flags)
{
	U_SHORT fl;

	if(!v)	return true;

	if(second)
		saveputc(save, ':')

	fl = save->sgfc->options->soft_linebreaks && (flags & SPLIT_SAVE);

	while(*v)
	{
		if(*v == '\\' || *v == ']' || (flags & PVT_COMPOSE && *v == ':'))
			saveputc(save, '\\');

		/* soft linebreak, if line is long and not in middle of multi-byte char */
		if(fl && (save->linelen > MAXTEXT_LINELEN) && (*v & 0xc0) != 0x80)
		{
			saveputc(save, '\\')	/* insert soft linebreak */
			saveputc(save, '\n')
		}

		if(!WriteChar(save, *v, flags & PVT_SIMPLE))
			return false;

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

static int WriteProperty(struct SaveInfo *save, struct TreeInfo *info, struct Property *prop)
{
	struct PropValue *v;
	char *p;
	bool do_tt;

	if(prop->flags & TYPE_GINFO)
	{
		if(!save->gi_written)
		{
			saveputc(save, '\n')
			saveputc(save, '\n')
		}
		save->gi_written = true;
	}
	else
		save->gi_written = false;


	p = prop->idstr;			/* write property ID */
	while(*p)
	{
		/* idstr is original from file -> may contain lowercase too */
		if(isupper((unsigned char)*p))
			saveputc(save, *p)
		p++;
	}

	do_tt = (info->GM == 1 && save->sgfc->options->pass_tt &&
			(info->bwidth <= 19) && (info->bheight <= 19) &&
			(prop->id == TKN_B || prop->id == TKN_W));

	v = prop->value;			/* write property value(s) */

	while(v)
	{
		saveputc(save, '[')

		if(do_tt && !v->value_len)
			WritePropValue(save, "tt", false, prop->flags);
		else
		{
			WritePropValue(save, v->value, false, prop->flags);
			WritePropValue(save, v->value2, true, prop->flags);
		}
		saveputc(save, ']')

		CheckLineLen(save)
		v = v->next;
	}

	if(prop->flags & TYPE_GINFO)
	{
		saveputc(save, '\n')
		if(prop->next)
		{
			if(!(prop->next->flags & TYPE_GINFO))
				saveputc(save, '\n')
		}
		else
			saveputc(save, '\n')
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

static int WriteNode(struct SaveInfo *save, struct TreeInfo *info, struct Node *n)
{
	struct Property *p;
	save->chars_in_node = 0;
	save->eol_in_node = 0;
	saveputc(save, ';')

	p = n->prop;
	while(p)
	{
		if((sgf_token[p->id].flags & PVT_CPLIST) && !save->sgfc->options->expand_cpl &&
		   (info->GM == 1))
			CompressPointList(save->sgfc, p);

		if(!WriteProperty(save, info, p))
			return false;

		p = p->next;
	}

	if(save->sgfc->options->node_linebreaks &&
	   ((save->eol_in_node && save->linelen > 0) ||
		(!save->eol_in_node &&
		  save->linelen > MAX_PREDICTED_LINELEN - save->chars_in_node)))
		saveputc(save, '\n')

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

static void SetRootProps(struct SaveInfo *save, struct TreeInfo *info, struct Node *r)
{
	if(r->parent)	/* isn't REAL root node */
		return;

	NewPropValue(save->sgfc, r, TKN_FF, "4", NULL, true);

	if(save->sgfc->options->encoding != OPTION_ENCODING_NONE)
		NewPropValue(save->sgfc, r, TKN_CA, "UTF-8", NULL, true);

	if(save->sgfc->options->add_sgfc_ap_property)
		NewPropValue(save->sgfc, r, TKN_AP, "SGFC", "1.18", true);

	if(info->GM == 1)
	{
		NewPropValue(save->sgfc, r, TKN_GM, "1", NULL, true);
		if(info->bwidth == 19 && info->bheight == 19)
			NewPropValue(save->sgfc, r, TKN_SZ, "19", NULL, true);
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

static int WriteTree(struct SaveInfo *save, struct TreeInfo *info,
					 struct Node *n, int newlines)
{
	if(newlines && save->linelen > 0)
		saveputc(save, '\n')

	SetRootProps(save, info, n);

	saveputc(save, '(')
	if(!WriteNode(save, info, n))
		return false;

	n = n->child;

	while(n)
	{
		if(n->sibling)
		{
			while(n)					/* write child + variations */
			{
				if(!WriteTree(save, info, n, 1))
					return false;
				n = n->sibling;
			}
		}
		else
		{
			if(!WriteNode(save, info, n))	/* write child */
				return false;
			n = n->child;
		}
	}

	saveputc(save, ')')

	if(newlines != 1)
		saveputc(save, '\n')

	return true;
}


/**************************************************************************
*** Function:	SaveSGF
***				writes the complete SGF tree to a file
*** Parameters: sgfc      ... pointer to SGFInfo structure
***				setup_sfh ... handler for writing to file or buffer
***				base_name ... filename/path of destination file
*** Returns:	true on success, false on error while writing file(s)
**************************************************************************/

bool SaveSGF(struct SGFInfo *sgfc, struct SaveFileHandler *(*setup_sfh)(void), const char *base_name)
{
	struct SaveInfo save = {sgfc, NULL, 0,0,0, false};
	struct Node *n;
	struct TreeInfo *info;
	const char *c;
	int nl = 0, i = 1;
	size_t name_buffer_size = strlen(base_name) + 14; /* +14 == "_99999999.sgf" + \0 */

	if(!(save.sfh = setup_sfh()))
		return false;

	char *name = SaveMalloc(name_buffer_size, "filename buffer");
	if(sgfc->options->split_file)
		snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);
	else
		strcpy(name, base_name);

	if(!(*save.sfh->open)(save.sfh, name, "wb"))
	{
		PrintError(FE_DEST_FILE_OPEN, sgfc, name);
		goto free_and_return_false;
	}

	if(sgfc->options->keep_head)
	{
		for(c = sgfc->buffer; c < sgfc->start; c++)
			if((*save.sfh->putc)(save.sfh, *c) == EOF)
				goto write_error;
		if((*save.sfh->putc)(save.sfh, '\n') == EOF)
			goto write_error;
	}

	save.linelen = 0;
	save.chars_in_node = 0;
	save.eol_in_node = 0;

	n = sgfc->root;
	info = sgfc->tree;

	while(n)
	{
		if(!WriteTree(&save, info, n, nl))
			goto write_error;

		nl = 2;
		n = n->sibling;
		info = info->next;

		if(sgfc->options->split_file && n)
		{
			(*save.sfh->close)(save.sfh, E_NO_ERROR);
			i++;
			snprintf(name, name_buffer_size, "%s_%03d.sgf", base_name, i);

			if(!(*save.sfh->open)(save.sfh, name, "wb"))
			{
				PrintError(FE_DEST_FILE_OPEN, sgfc, name);
				goto free_and_return_false;
			}
		}
	}

	(*save.sfh->close)(save.sfh, E_NO_ERROR);
	free(name);
	free(save.sfh);
	return true;

write_error:
	(*save.sfh->close)(save.sfh, FE_DEST_FILE_WRITE);
	PrintError(FE_DEST_FILE_WRITE, sgfc, name);
free_and_return_false:
	free(name);
	free(save.sfh);
	return false;
}
