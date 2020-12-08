/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 error.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "all.h"
#include "protos.h"


/* Error reporting hooks */
bool (*print_error_handler)(U_LONG, struct SGFInfo *, va_list) = PrintErrorHandler;
void (*print_error_output_hook)(struct SGFCError *) = PrintErrorOutputHook;
void (*oom_panic_hook)(const char *) = ExitWithOOMError;


static const char *error_mesg[] =
{
		"unknown command '%s' (-h for help)\n",
		"unknown command line option '%c' (-h for help)\n",
		"could not open source file '%s' - ",
		"could not read source file '%s' - ",
		"could not allocate %s (not enough memory)\n",
/* 5 */
		"possible SGF data found in front of game-tree (before '(;')\n",
		"no SGF data found - start mark '(;' missing?\n",
		"illegal char(s) found: ",
		"variation nesting incomplete (missing ')')\n",
		"unexpected end of file\n",
/* 10 */
		"property identifier too long - more than 100 chars (deleted)\n",
		"empty variation found (ignored)\n",
		"property <%s> may have only ONE value (other values deleted)\n",
		"illegal <%s> value deleted: ",
		"illegal <%s> value corrected; new value: [%s], old value: ",
/* 15 */
		"lowercase char not allowed in property identifier\n",
		"empty <%s> value %s (deleted)\n",
		"illegal root property <%s> found (assuming %s)\n",
		"game stored in tree %d is not Go. Cannot check move & position type"
		" -> errors will not get corrected!\n",
		"property <%s> without any values found (ignored)\n",
/* 20 */
		"illegal variation start found (ignored)\n",
		"$00 byte detected (replaced with space) - binary file?\n",
		"property <%s> expects compose type value (value deleted): ",
		"move in root node found (split node into two)\n",
		"illegal <%s> value corrected; new value: [%s:%s], old value: ",
/* 25 */
		"could not open destination file '%s' - ",
		"could not write destination file '%s' - ",
		"property <%s> already exists (%s)\n",
		"%sproperty <%s> deleted\n",
		"setup and move properties mixed within a node (%s)\n",
/* 30 */
		"property identifier consists of more than 2 uppercase letters: <%s>\n",
		"root property <%s> outside root node (deleted)\n",
		"gameinfo property <%s> has illegal format %s - value: ",
		"file not saved (because of critical errors)\n",
		"unknown property <%s> %s\n",
/* 35 */
		"missing semicolon at start of game-tree (detection might be wrong [try -b2])\n",
		"black and white move within a node (split into two nodes)\n",
		"%s <%s> position not unique ([partially] deleted) - value(s): ",
		"AddStone <%s> has no effect ([partially] deleted) - value(s): ",
		"property <%s> is not defined in FF[%d] (%s)\n",
/* 40 */
		"annotation property <%s> contradicts previous property (deleted)\n",
		"combination of <%s> found (converted to <%s>)\n",
		"move annotation <%s> without a move in same node (deleted)\n",
		"game info entry <%s> outside game-info node (line:%d col:%d) (deleted)\n",
		"different file formats stored in one file (may cause troubles with some applications)\n",
/* 45 */
		"unknown file format FF[%d] (only able to handle files up to FF[4])\n",
		"square board size in rectangular definition (corrected)\n",
		"no source file specified (-h for help)\n",
		"bad command line option parameter '%s' (-h for help)\n",
		"board size too big (corrected to %dx%d)\n",
/* 50 */
		"used feature is not defined in FF[%d] (parsing done anyway)\n",
		"<VW> property: %s (%s)\n",
		"different game types stored in one file (may cause troubles with some applications)\n",
		"values without property id found (deleted)\n",
		"empty node deleted\n",
/* 55 */
		"possible incorrect variation level cannot be corrected\n",
		"variation level corrected\n",
		"forbidden move found (played on a point occupied by another stone)\n",
		"obsolete <KI> property found: %s\n",
		"file contains more than one game tree\n",
/* 60 */
		"value of HA property differs from number of setup stones\n",
		"setup stones in main line found (outside root node)\n",
		"two successive moves have the same color\n",
		"cannot reorder variations: too many variations\n",
		"FF4 style pass value '[]' in older format found (corrected)\n",
/* 65 */
		"node outside variation found. Missing '(' assumed.\n",
		"illegal chars after variation start '(' found. Missing ';' assumed.\n",
		"unknown command line option '%s' (-h for help)\n",
		"unknown or inconvertible encoding given as parameter in %s: '%s'\n",
		"unknown iconv error during encoding phase encountered - byte offset: %ld\n",
/* 70 */
		"encoding errors detected (faulty bytes ignored) - byte offset: %ld\n",
		"unknown encoding '%s' - falling back to default encoding '%s'\n",
		"charset encoding detection went wrong! Please use --encoding to override.\n",
		"different charset encodings stored in one file (will cause troubles with applications)\n",
		"different encodings in one file detected. Use option -E2/3 to parse this file\n",
};


/* internal data for error.c functions (currently only for PrintErrorHandler) */
/* Used instead of local static variables */
#define ACCUMULATE_SIZE 80

struct ErrorC_internal {
	U_LONG last_row;	/* type & position of last error */
	U_LONG last_col;
	U_LONG last_type;

	char accumulate[ACCUMULATE_SIZE];
	U_LONG acc_count;
	U_LONG acc_row;		/* type & position of last accumulate error */
	U_LONG acc_col;
	U_LONG acc_type;

	bool error_seen[MAX_ERROR_NUM];	/* used for E_ONLY_ONCE */
};


/**************************************************************************
*** Function:	SetupErrorC_internal
***				Allocate and initialize internal data structure local to error.c
*** Parameters: -
*** Returns:	pointer to internal structure
**************************************************************************/

struct ErrorC_internal *SetupErrorC_internal(void)
{
	struct ErrorC_internal *errc = SaveCalloc(sizeof(struct ErrorC_internal), "static error.c struct");
	errc->acc_type = E_NO_ERROR;
	return errc;
}


/**************************************************************************
*** Function:	PrintError
***				Variadic wrapper around PrintErrorHandler
**************************************************************************/

int PrintError(U_LONG type, struct SGFInfo *sgfc, ...) {
	int result = 0;

	va_list arglist;
	va_start(arglist, sgfc);
	if (print_error_handler)
		result = (*print_error_handler)(type, sgfc, arglist);
	va_end(arglist);
	return result;
}


/**************************************************************************
*** Function:	ExitWithOOMError
***				Special error printer in case when memory runs out
***				Panics: does not return; does not free up resources; just dies
***				Might be called, when SGFInfo is not properly set up yet.
**************************************************************************/

void ExitWithOOMError(const char *detail)
{
	int err_num = FE_OUT_OF_MEMORY & M_ERROR_NUM;
	fprintf(E_OUTPUT, "Fatal error %d: ", err_num);
	fprintf(E_OUTPUT, error_mesg[err_num - 1], detail);
	exit(20);
}


/**************************************************************************
*** Function:	PrintErrorHandler
***				The default handler for printing error messages.
***				Performs several calculations/aggregation etc. if required
*** Parameters: type ... error code
***				additional parameters as needed:
***				[position] [accumulate] [acc_size]
*** Returns:	true: error printed
***				false: error disabled
**************************************************************************/

bool PrintErrorHandler(U_LONG type, struct SGFInfo *sgfc, va_list arglist) {
	int print_c = 0;
	struct SGFCError error = {0, NULL, 0, 0, 0};
	char *error_msg_buffer = NULL, *val_pos = NULL, *illegal = NULL;
	U_LONG row = 0, col = 0;
	int illegal_count;
	va_list argtmp;

	if(type & E_ERROR4)
	{
		if(sgfc->info->FF >= 4)		type |= E_ERROR;
		else						type |= E_WARNING;
	}
	if(type & E_WARNING_STRICT)
	{
		if(sgfc->options->strict_checking)	type |= E_ERROR;
		else								type |= E_WARNING;
	}

	if(type & E_ONLY_ONCE)
	{
		if(sgfc->_error_c->error_seen[(type & M_ERROR_NUM) - 1])
			return false;
		sgfc->_error_c->error_seen[(type & M_ERROR_NUM) - 1] = true;
	}

	if((!sgfc->options->error_enabled[(type & M_ERROR_NUM)-1] && !(type & E_FATAL_ERROR)
		&& type != E_NO_ERROR) || (!sgfc->options->warnings && (type & E_WARNING)))
	{								/* error message enabled? */
		sgfc->ignored_count++;
		return false;
	}

	if(type & E_SEARCHPOS)			/* get pointer to position if required */
	{
		row = va_arg(arglist, U_LONG);
		col = va_arg(arglist, U_LONG);

		if(row == sgfc->_error_c->last_row && col == sgfc->_error_c->last_col &&
		   type == sgfc->_error_c->last_type && type & E_DEL_DOUBLE)
			/* eliminate double messages generated by compressed point lists */
			return false;

		sgfc->_error_c->last_row = row;
		sgfc->_error_c->last_col = col;
		sgfc->_error_c->last_type = type;
	}
	else
		sgfc->_error_c->last_type = E_NO_ERROR;

	// FIXME: maybe avoid duplicate error messages (compressed point list, delete CTRL byte)

	if((type & E_ACCUMULATE))			/* accumulate error messages? */
	{
		if(va_arg(arglist, int))		/* true: accumulate */
		{
			if(sgfc->_error_c->acc_count)	/* any errors already accumulated? */
			{
				/* different error at follow-up position? -> flush */
				if((sgfc->_error_c->acc_row != row) ||
				   (sgfc->_error_c->acc_col + sgfc->_error_c->acc_count != col) ||
				   ((sgfc->_error_c->acc_type & M_ERROR_NUM) != (type & M_ERROR_NUM)))
				{
					PrintError(sgfc->_error_c->acc_type, sgfc,
							   sgfc->_error_c->acc_row, sgfc->_error_c->acc_col, false);
					sgfc->_error_c->acc_row = row;	/* set new */
					sgfc->_error_c->acc_col = col;
					sgfc->_error_c->acc_type = type;
				}
			}
			else								/* first error */
			{
				sgfc->_error_c->acc_type = type;	/* set data */
				sgfc->_error_c->acc_row = row;
				sgfc->_error_c->acc_col = col;
			}

			illegal = va_arg(arglist, char *);
			if(type & E_MULTIPLE)	illegal_count = va_arg(arglist, U_LONG);
			else					illegal_count = 1;

			/* illegal_count might overflow or even be much larger than ACCUMULATE_SIZE */
			while(sgfc->_error_c->acc_count + illegal_count >= ACCUMULATE_SIZE)
			{
				if(sgfc->_error_c->acc_count < ACCUMULATE_SIZE)
				{
					size_t chunk = ACCUMULATE_SIZE - sgfc->_error_c->acc_count;
					strnpcpy(&sgfc->_error_c->accumulate[sgfc->_error_c->acc_count], illegal, chunk);
					sgfc->_error_c->acc_count += chunk;
					illegal_count -= chunk;
					illegal += chunk;
				}
				/* flush accumulate buffer (sets acc_count=0) */
				PrintError(sgfc->_error_c->acc_type, sgfc,
						   sgfc->_error_c->acc_row, sgfc->_error_c->acc_col, false);
			}
			/* any remainders should now be small enough to fit */
			if(illegal_count)
			{
				strnpcpy(&sgfc->_error_c->accumulate[sgfc->_error_c->acc_count], illegal, illegal_count);
				sgfc->_error_c->acc_count += illegal_count;
			}

			va_end(arglist);
			return true;
		}
		/* false: don't accumulate, print it */
		print_c = 1;
	}
	else								/* not an ACCUMULATE type */
	if(sgfc->_error_c->acc_count)	/* any errors waiting? */
		/* flush buffer and continue ! */
		PrintError(sgfc->_error_c->acc_type, sgfc,
				   sgfc->_error_c->acc_row, sgfc->_error_c->acc_col, false);

	if(type == E_NO_ERROR)
		return true;

	if(type & E_SEARCHPOS)				/* print position if required */
	{
		error.row = row;
		error.col = col;
	}

	if(type & E_ERROR)
		sgfc->error_count++;
	else if(type & E_WARNING)
		sgfc->warning_count++;
	if(type & E_CRITICAL)
		sgfc->critical_count++;

	/* populate SGFCError structure to pass to print_error_output_hook */
	size_t malloc_size = 1; /* \0 byte */

	if(print_c)
		malloc_size += sgfc->_error_c->acc_count + 4; /* 3 Bytes ""\n + 1 reserve */

	if(type & E_VALUE)			/* print a property value ("[value]\n") */
	{
		val_pos = va_arg(arglist, char *);
		malloc_size += strlen(val_pos) + 3; /* + '[]\n' bytes */
	}

	va_copy(argtmp, arglist);
	size_t size = vsnprintf(NULL, 0, error_mesg[(type & M_ERROR_NUM)-1], argtmp);
	va_end(argtmp);
	malloc_size += size;

	error_msg_buffer = (char *)malloc(malloc_size);
	if(!error_msg_buffer)
		error.message = "out of memory (while printing error)\n";
	else
	{
		char *msg_cursor = error_msg_buffer + size;
		vsnprintf(error_msg_buffer, size+1, error_mesg[(type & M_ERROR_NUM)-1], arglist);
		if(print_c)					/* print accumulated string? */
		{
			*msg_cursor++ = '"';
			/* _error_c->accumulate string is _not_ terminated with '\0' */
			strncpy(msg_cursor, sgfc->_error_c->accumulate, sgfc->_error_c->acc_count);
			msg_cursor += sgfc->_error_c->acc_count;
			sgfc->_error_c->acc_count = 0;
			*msg_cursor++ = '"';
			*msg_cursor++ = '\n';
		}
		if(val_pos)
		{
			*msg_cursor++ = '[';
			strnpcpy(msg_cursor, val_pos, strlen(val_pos));
			msg_cursor += strlen(val_pos);
			*msg_cursor++ = ']';
			*msg_cursor++ = '\n';
		}
		*msg_cursor = 0;
		error.message = error_msg_buffer;
	}

	if(type & E_ERRNO)			/* print DOS error message? */
		error.lib_errno = errno;

	error.error = type;
	if(print_error_output_hook)
		(*print_error_output_hook)(&error);		/* call output hook function */
	free(error_msg_buffer);
	return true;
}


/**************************************************************************
*** Function:	PrintErrorOutputHook
***				Prints an error message to E_OUTPUT (stdout)
*** Parameters: error ... structure that contains error information
*** Returns:    -
**************************************************************************/

void PrintErrorOutputHook(struct SGFCError *error)
{
	CommonPrintErrorOutputHook(error, E_OUTPUT);
}

void CommonPrintErrorOutputHook(struct SGFCError *error, FILE *stream)
{
	if(error->row && error->col)		/* print position if required */
		fprintf(stream, "Line:%lu Col:%lu - ", error->row, error->col);

	switch(error->error & M_ERROR_TYPE)
	{
		case E_FATAL_ERROR:	fprintf(stream, "Fatal error %d", (int)(error->error & M_ERROR_NUM));
			break;
		case E_ERROR:		fprintf(stream, "Error %d", (int)(error->error & M_ERROR_NUM));
			break;
		case E_WARNING:		fprintf(stream, "Warning %d", (int)(error->error & M_ERROR_NUM));
			break;
	}

	if(error->error & E_CRITICAL)
		fprintf(stream, " (critical): ");
	else
		fprintf(stream, ": ");

	fputs(error->message, stream);

	if(error->error & E_ERRNO)			/* print DOS error message? */
	{
		char *err;
		err = strerror(error->lib_errno);
		if(err)
			fprintf(stream, "%s\n", err);
		else
			fprintf(stream, "error code: %d\n", error->lib_errno);
	}
}
