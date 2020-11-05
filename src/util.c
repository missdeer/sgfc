/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 util.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "all.h"
#include "protos.h"


/* Error reporting hooks */
int (*print_error_handler)(U_LONG, struct SGFInfo *, va_list) = PrintErrorHandler;
void (*print_error_output_hook)(struct SGFCError *) = PrintErrorOutputHook;


static const char *error_mesg[] =
{
	"unknown command '%s' (-h for help)\n",
	"unknown command line option '%c' (-h for help)\n",
	"could not open source file '%s' - ",
	"could not read source file '%s' - ",
	"could not allocate %s (not enough memory)\n",
/* 5 */
	"possible SGF data found in front of game-tree (before '(;')\n",
	"could not find start mark '(;' - no SGF data found\n",
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
	"$00 byte deleted - binary file?\n",
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
	"missing semicolon at start of game-tree (detection might be wrong [use -b2 in that case])\n",
	"black and white move within a node (split into two nodes)\n",
	"%s <%s> position not unique ([partially] deleted) - value(s): ",
	"AddStone <%s> has no effect ([partially] deleted) - value(s): ",
	"property <%s> is not defined in FF[%d] (%s)\n",
/* 40 */
	"annotation property <%s> contradicts previous property (deleted)\n",
	"combination of <%s> found (converted to <%s>)\n",
	"move annotation <%s> without a move in node (deleted)\n",
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
/*65 */
	"node outside variation found. Missing '(' assumed.\n",
	"illegal chars after variation start '(' found. Missing ';' assumed.\n",
	"unknown command line option '%s' (-h for help)\n"
};


/* internal data for util.c functions (currently only for PrintErrorHandler) */
/* Used instead of local static variables */
struct UtilC_internal {
	char *last_pos;
	char *illegal;
	int ill_count;
	U_LONG ill_type;
	U_LONG last_type;
	bool error_seen[MAX_ERROR_NUM];	/* used for E_ONLY_ONCE */
};


/**************************************************************************
*** Function:	SetupUtilC_internal
***				Allocate and initialize internal data structure local to util.c
*** Parameters: -
*** Returns:	pointer to internal structure
**************************************************************************/

struct UtilC_internal *SetupUtilC_internal(void)
{
	struct UtilC_internal *utilc;
	SaveMalloc(struct UtilC_internal *, utilc, sizeof(struct UtilC_internal), "static util.c struct")
	memset(utilc, 0, sizeof(struct UtilC_internal));
	utilc->ill_type = E_NO_ERROR;
	return utilc;
}


/**************************************************************************
*** Function:	SearchPos
***				Calculate the position in rows/columns within a buffer
*** Parameters: c ... current position
***				sgfc ... for start/end of buffer pointers
***				x ... result: #columns
***				y ... result: #rows
*** Returns:	- (result stored in x,y)
**************************************************************************/

void SearchPos(const char *c, struct SGFInfo *sgfc, int *x, int *y)
{
	char newline = 0;
	char *s = sgfc->buffer;

	*y = 1;
	*x = 1;

	while(s != c && s < sgfc->b_end)
	{
		if(*s == '\r' || *s == '\n')		/* linebreak char? */
		{
			if(newline && newline != *s)	/* different from preceding char? */
				newline = 0;				/* ->yes: no real linebreak */
			else
			{
				(*y)++;						/* lines += 1 */
				*x = 1;						/* reset column */
				newline = *s;
			}
		}
		else
		{
			newline = 0;					/* no linebreak char */
			(*x)++;							/* column++ */
		}

		s++;
	}
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

int __attribute__((noreturn)) ExitWithOOMError(char *detail)
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

int PrintErrorHandler(U_LONG type, struct SGFInfo *sgfc, va_list arglist) {
	int print_c = 0;
	char *pos = NULL;
	struct SGFCError error = {0, NULL, NULL, 0, 0, 0};

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
		if(sgfc->_util_c->error_seen[(type & M_ERROR_NUM)-1])
			return false;
		sgfc->_util_c->error_seen[(type & M_ERROR_NUM)-1] = true;
	}

	if((!sgfc->options->error_enabled[(type & M_ERROR_NUM)-1] && !(type & E_FATAL_ERROR)
		 && type != E_NO_ERROR) || (!sgfc->options->warnings && (type & E_WARNING)))
	{								/* error message enabled? */
		sgfc->ignored_count++;
		return false;
	}

	if(type & E_SEARCHPOS)			/* get pointer to position if required */
	{
		pos = va_arg(arglist, char *);
		error.buffer = pos;

		if(pos == sgfc->_util_c->last_pos &&
		   type == sgfc->_util_c->last_type && type & E_DEL_DOUBLE)
			/* eliminate double messages generated by compressed point lists */
			return false;
		else
		{
			sgfc->_util_c->last_pos = pos;
			sgfc->_util_c->last_type = type;
		}
	}
	else
		sgfc->_util_c->last_type = E_NO_ERROR;


	if((type & E_ACCUMULATE))			/* accumulate error messages? */
	{
		if(va_arg(arglist, int))		/* true: accumulate */
		{
			if(sgfc->_util_c->ill_count)	/* any errors already accumulated? */
			{
				/* succeed? */
				if((sgfc->_util_c->illegal + sgfc->_util_c->ill_count != pos) ||
				  ((sgfc->_util_c->ill_type & M_ERROR_NUM)!=(type & M_ERROR_NUM)))
				{	/* no -> flush */
					PrintError(sgfc->_util_c->ill_type, sgfc, sgfc->_util_c->illegal, false);
					sgfc->_util_c->illegal = pos;	/* set new */
					sgfc->_util_c->ill_type = type;
				}
			}
			else								/* first error */
			{
				sgfc->_util_c->ill_type = type;	/* set data */
				sgfc->_util_c->illegal = pos;
			}

			if(type & E_MULTIPLE)				/* more than one char? */
				sgfc->_util_c->ill_count += va_arg(arglist, int);	/* get size */
			else
				sgfc->_util_c->ill_count++;		/* else +1 */

			va_end(arglist);
			return true;
		}
		else								/* false: print it */
			print_c = 1;
	}
	else								/* not an ACCUMULATE type */
		if(sgfc->_util_c->ill_count)	/* any errors waiting? */
			/* flush buffer and continue ! */
			PrintError(sgfc->_util_c->ill_type, sgfc, sgfc->_util_c->illegal, false);

	if(type & E_SEARCHPOS)				/* print position if required */
		SearchPos(pos, sgfc, &error.x, &error.y);

	if(type & E_ERROR)
		sgfc->error_count++;
	else if(type & E_WARNING)
		sgfc->warning_count++;
	if(type & E_CRITICAL)
		sgfc->critical_count++;


	if(type == E_NO_ERROR)
		return true;

	char *error_msg_buffer = NULL;
	char *pos2 = NULL;
	va_list argtmp;

	va_copy(argtmp, arglist);
	size_t size = vsnprintf(NULL, 0, error_mesg[(type & M_ERROR_NUM)-1], argtmp);
	va_end(argtmp);
	size_t malloc_size = size + 1; /* \0 byte */

	if(type & E_VALUE)			/* print a property value ("[value]\n") */
	{
		pos2 = SkipText(sgfc, pos, NULL, ']', 0);
		if (pos2 >= pos)
			malloc_size += pos2 - pos + 2; /* pos2 inclusive + '\n' byte */
	}

	if(print_c)
		malloc_size += sgfc->_util_c->ill_count + 4; /* 3 Bytes ""\n + 1 reserve */
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
			/* string is not terminated with '\0'! */
			for(; sgfc->_util_c->ill_count; sgfc->_util_c->ill_count--)
			{
				*msg_cursor++ = *sgfc->_util_c->illegal++;
			}
			*msg_cursor++ = '"';
			*msg_cursor++ = '\n';
		}
		if(pos2)
		{
			char *valpos = pos;
			while(valpos <= pos2)
				*msg_cursor++ = *valpos++;
			*msg_cursor++ = '\n';
		}
		*msg_cursor = 0;
		error.message = error_msg_buffer;
	}

	if(type & E_ERRNO)			/* print DOS error message? */
		error.lib_errno = errno;

	error.error = type;
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

void PrintErrorOutputHook(struct SGFCError *error) {
	if(error->x && error->y)		/* print position if required */
		fprintf(E_OUTPUT, "Line:%d Col:%d - ", error->y, error->x);

	switch(error->error & M_ERROR_TYPE)
	{
		case E_FATAL_ERROR:	fprintf(E_OUTPUT, "Fatal error %d", (int)(error->error & M_ERROR_NUM));
							break;
		case E_ERROR:		fprintf(E_OUTPUT, "Error %d", (int)(error->error & M_ERROR_NUM));
							break;
		case E_WARNING:		fprintf(E_OUTPUT, "Warning %d", (int)(error->error & M_ERROR_NUM));
							break;
	}

	if(error->error & E_CRITICAL)
		fprintf(E_OUTPUT, " (critical): ");
	else
		if(error->error != E_NO_ERROR)
			fprintf(E_OUTPUT, ": ");

	if(error->error != E_NO_ERROR)
	{
		fputs(error->message, E_OUTPUT);
		if(error->error & E_ERRNO)			/* print DOS error message? */
		{
			char *err;
			err = strerror(error->lib_errno);
			if(err)
				fprintf(E_OUTPUT, "%s\n", err);
			else
				fprintf(E_OUTPUT, "error code: %d\n", error->lib_errno);
		}
	}
}


/**************************************************************************
*** Function:	EncodePosChar // DecodePosChar
***				Encodes or decodes a position character
*** Parameters: c ... number or character
*** Returns:	false for error, a-zA-Z or 1-52
**************************************************************************/

char EncodePosChar(int c)
{
	if(c <= 0 || c > 52)
		return false;
	if(c <= 26)
		return (char)('a'+c-1);
	else
		return (char)('A'+c-27);
}

int DecodePosChar(char c)
{
	if(islower(c))
		return c-'a'+1;
	else
		if(isupper(c))
			return c-'A'+27;
		else
			return false;
}


/**************************************************************************
*** Function:	f_AddTail
***				Adds a node at the tail of a double-linked list
*** Parameters: h	... list header (first/last)
***				n	... node to add
*** Returns:	-
**************************************************************************/

void f_AddTail(struct ListHead *h, struct ListNode *n)
{
	n->next = NULL;

	if(h->last)
		h->last->next = n;

	n->prev = h->last;
	h->last = n;

	if(!h->first)
		h->first = n;
}

/**************************************************************************
*** Function:	f_Enqueue
***				Inserts a node into a double-linked list according to its
***				priority (same priorities: at the tail)
*** Parameters: h	... list header (first/last)
***				n	... node to insert
*** Returns:	-
**************************************************************************/

void f_Enqueue(struct ListHead *h, struct ListNode *n)
{
	struct ListNode *i;

	i = h->first;

	while(i)
	{
		if(i->priority < n->priority)
		{
			if(i->prev)
				i->prev->next = n;
			else
				h->first = n;

			n->prev = i->prev;
			n->next = i;
			i->prev = n;
			return;
		}

		i = i->next;
	}

	f_AddTail(h, n);		/* end of list reached -> add node at tail */
}

/**************************************************************************
*** Function:	f_Delete
***				Deletes (unlinks) an entry of a double-linked list
*** Parameters: h	... list header (first/last)
***				n	... node to delete
*** Returns:	-
**************************************************************************/

void f_Delete(struct ListHead *h, struct ListNode *n)
{
	if(n->prev)
		n->prev->next = n->next;
	else
		h->first = n->next;

	if(n->next)
		n->next->prev = n->prev;
	else
		h->last = n->prev;
}


/**************************************************************************
*** Function:	strnccmp
***				String compare, not case sensitive
*** Parameters: a, b ... strings to be compared
***				len  ... number of bytes to compare or 0
*** Returns:	true=strings not equal, false= equal
**************************************************************************/

int strnccmp(char *a, char *b, size_t len)
{
	if(!len)
		len = strlen(a) + 1;

	while(*a && *b && len)
	{
		if(toupper(*a) != toupper(*b))
			return true;
		a++;
		b++;
		len--;
	}

	if(len && (*a || *b))
		return true;

	return false;
}


/**************************************************************************
*** Function:	KillChars
***				Deletes selected char-set out of a given string
*** Parameters: value ... string
***				kill ... type of chars to be removed
***						 C_ISSPACE, C_NOT_ISALPHA or C_NOTinSET/C_inSET
***				cset ... additional set of chars to be tested
***						 (kill has to be one of C_NOTinSET or C_inSET)
*** Returns:	number of chars removed from string
**************************************************************************/

U_LONG KillChars(char *value, U_SHORT kill, char *cset)
{
	U_LONG faulty = 0, err = 0;
	char *c, *d;

	for(c = d = value; *c; c++)
	{
		if(((kill & C_ISSPACE) && isspace(*c)) ||
		   ((kill & C_NOT_ISALPHA) && !isalpha(*c)))
			err = 1;
		else
			if(kill & C_NOTinSET)
			{
				if(!strchr(cset, *c))
					err = 1;
			}
			else
				if(kill & C_inSET)
					if(strchr(cset, *c))
						err = 1;
		if(err)
		{
			faulty++;
			err = 0;
		}
		else
			*d++ = *c;
	}
	*d = 0;			/* end mark */

	return faulty;
}


/**************************************************************************
*** Function:	TestChars
***				Tests if string contains specified char-set
***				Spaces are ignored (can't be tested)
*** Parameters: value ... string
***				test ... test for chars (C_ISALPHA or C_inSET/C_NOTinSET)
***				cset ... char-set to be searched in value
***						 (test has to be one of C_inSET or C_NOTinSET)
*** Returns:	number of selected chars found in value
**************************************************************************/

U_LONG TestChars(char *value, U_SHORT test, char *cset)
{
	U_LONG faulty = 0;
	char *c;

	for(c = value; *c; c++)
	{
		if(isspace(*c))		/* WhiteSpace are ignored !! */
			continue;

		if((test & C_ISALPHA) && isalpha(*c))
			faulty++;
		else
			if(test & C_NOTinSET)
			{
				if(!strchr(cset, *c))
					faulty++;
			}
			else
				if(test & C_inSET)
					if(strchr(cset, *c))
						faulty++;
	}

	return faulty;
}


/**************************************************************************
*** Function:	FindProperty
***				Searches property within a node
*** Parameters: n ... node to be searched
***				id ... property id
*** Returns:	pointer to property or NULL
**************************************************************************/

struct Property *FindProperty(struct Node *n, token id)
{
	struct Property *p;

	for(p = n->prop; p; p=p->next)
		if(p->id == id)
			break;

	return p;
}


/**************************************************************************
*** Function:	DelPropValue
***				Deletes a value of a property
*** Parameters: p ... property to which value belongs
***				v ... value to be deleted
*** Returns:	v->next
**************************************************************************/

struct PropValue *DelPropValue(struct Property *p, struct PropValue *v)
{
	struct PropValue *next;

	if (!v)
		return NULL;

	if(v->value)		free(v->value);
	if(v->value2)		free(v->value2);

	next = v->next;

	Delete(&p->value, v);
	free(v);
	return next;
}


/**************************************************************************
*** Function:	DelProperty
***				Deletes a property
*** Parameters: n ... node which contains property
***				p ... property to be deleted
*** Returns:	p->next
**************************************************************************/

struct Property *DelProperty(struct Node *n, struct Property *p)
{
	struct Property *next;
	struct PropValue *v;

	v = p->value;		/* del property values */
	while(v)
		v = DelPropValue(p, v);

	next = p->next;		/* remove property from node */

	if(n)
		Delete(&n->prop, p);

	if(p->id == TKN_UNKNOWN)
		free(p->idstr);

	free(p);
	return next;
}


/**************************************************************************
*** Function:	DelNode
***				Deletes empty node if
***				- node has no siblings
***				- has siblings (or is root) but has max. one child
*** Parameters: sgfc ... pointer to SGFInfo
***				n	 ... node that should be deleted
***				error_code ... error code to report (while deleting) or E_NO_ERROR
*** Returns:	n->next
**************************************************************************/

struct Node *DelNode(struct SGFInfo *sgfc, struct Node *n, U_LONG error_code)
{
	struct Node *p, *h;
	struct Property *i;

	p = n->parent;

	if((p && (n->sibling || (p->child != n))) || !p)
	{
		if(n->child)
			if(n->child->sibling)
				return n->next;
	}

	if(error_code != E_NO_ERROR)
		PrintError(error_code, sgfc, n->buffer);

	if(n->prop)						/* delete properties */
	{
		i = n->prop;
		while(i)
			i = DelProperty(n, i);
	}

	if(!p)							/* n is a root node */
	{
		if(n->child)
		{
			n->child->sibling = n->sibling;
			n->child->parent = NULL;
		}

		if(sgfc->root == n)			/* n is first root */
		{
			if(n->child) sgfc->root = n->child;
			else sgfc->root = n->sibling;
		}
		else
		{							/* n is subsequent root */
			h = sgfc->root;
			while(h->sibling != n)
				h = h->sibling;

			if(n->child)	h->sibling = n->child;
			else			h->sibling = n->sibling;
		}
	}
	else							/* n is root of subtree */
	{
		if(n->sibling || (p->child != n))	/* empty node has siblings */
		{
			if(n->child)			/* child has no siblings! */
			{
				n->child->parent = p;
				n->child->sibling = n->sibling;
			}

			if(p->child == n)		/* n is first sibling */
			{
				if(n->child)		p->child = n->child;
				else				p->child = n->sibling;
			}
			else
			{						/* n is subsequent sibling */
				h = p->child;
				while(h->sibling != n)
					h = h->sibling;

				if(n->child)		h->sibling = n->child;
				else				h->sibling = n->sibling;
			}
		}
		else						/* empty node has no siblings */
		{							/* but child may have siblings */
			p->child = n->child;
			h = n->child;
			while(h)				/* set new parent */
			{
				h->parent = p;
				h = h->sibling;
			}
		}
	}

	h = n->next;
	Delete(&sgfc->first, n);
	free(n);

	return h;
}


/**************************************************************************
*** Function:	NewPropValue
***				Adds (or sets) a new property value
***				(adds property to node if necessary)
*** Parameters: sgfc	... pointer to SGFInfo structure
***				n		... node
***				id		... property id
***				value	... first value
***				value2	... second value (if compose type) or NULL
***				unique	... true - old values get deleted
*** Returns:	pointer to property
**************************************************************************/

struct Property *NewPropValue(struct SGFInfo *sgfc, struct Node *n, token id,
							  const char *value, const char *value2, int unique)
{
	struct Property *p;
	struct PropValue *v;
	size_t size1, size2;

	p = FindProperty(n, id);
	if(p)
	{
		if(unique)
			for(v = p->value; v; v = DelPropValue(p, v));
	}
	else
		p = AddProperty(n, id, NULL, NULL);

	size1 = strlen(value);
	size2 = value2 ? strlen(value2) : 0;
	AddPropValue(sgfc, p, NULL, value, size1, value2, size2);

	return p;
}


/**************************************************************************
*** Function:	CalcGameSig
***				Calculates game signature as proposed by Dave Dyer
*** Parameters: ti 		... first tree info structure
*** 			buffer	... buffer to write game signature into; at least 14 bytes!
*** Returns:	true ... signature calculated // false ... could not calc signature
**************************************************************************/

int CalcGameSig(struct TreeInfo *ti, char *buffer)
{
	int i;
	struct Node *n;
	struct Property *p;

	if(ti->GM != 1)
		return false;

	strcpy(buffer, "------ ------");
	i = 0;
	n = ti->root;

	while(n && i < 71)
	{
		p = FindProperty(n, TKN_B);
		if(!p)
			p = FindProperty(n, TKN_W);
		n = n->child;
		if(!p)
			continue;
		i++;
		switch(i)
		{
			case 20:
			case 40:
			case 60:	buffer[i/10-2] = p->value->value[0];
						buffer[i/10-1] = p->value->value[1];
						break;
			case 31:
			case 51:
			case 71:	buffer[i/10+4] = p->value->value[0];
						buffer[i/10+5] = p->value->value[1];
						break;
			default:	break;
		}
	}
	return true;
}
