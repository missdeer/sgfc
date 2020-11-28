/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 util.c
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
	return (char)('A'+c-27);
}

int DecodePosChar(char c)
{
	if(islower(c))
		return c-'a'+1;
	if(isupper(c))
		return c-'A'+27;
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
*** Function:	SaveMalloc
***				malloc() + error handling (i.e. printing error + failing)
*** Parameters: size ... size of memory to allocate
***				err	 ... error message
*** Returns:	pointer to memory (or termination in case of error)
**************************************************************************/

void *SaveMalloc(size_t size, const char *err)
{
	void *mem = malloc(size);
	if(!mem)
	{
		(*oom_panic_hook)(err); /* function will not return */
		/* exit() will never be reached; safe-guard and hint for linting */
		exit(20);
	}
	return mem;
}


/**************************************************************************
*** Function:	SaveCalloc
***				calloc() + error handling (i.e. printing error + failing)
*** Parameters: size ... size of memory to allocate
***				err	 ... error message
*** Returns:	pointer to memory (or termination in case of error)
**************************************************************************/

void *SaveCalloc(size_t size, const char *err)
{
	void *mem = calloc(size, 1);
	if(!mem)
	{
		(*oom_panic_hook)(err); /* function will not return */
		/* exit() will never be reached; safe-guard and hint for linting */
		exit(20);
	}
	return mem;
}


/**************************************************************************
*** Function:	SaveDupString
***				Safely duplicate a string (possibly not \0 terminated)
*** Parameters: src ... source buffer
***				len	 ... size of buffer
***				err	 ... error message
*** Returns:	pointer to \0-terminated duplicate (or termination in case of error)
**************************************************************************/

char *SaveDupString(const char *src, size_t len, const char *err)
{
	if(!len)
		len = strlen(src);
	char *dst = SaveMalloc(len+1, err);
	memcpy(dst, src, len);
	*(dst+len) = 0;	/* 0-terminate */
	return dst;
}


/**************************************************************************
*** Function:	strnccmp
***				String compare, not case sensitive
*** Parameters: a, b ... strings to be compared
***				len  ... number of bytes to compare or 0 (=use strlen)
*** Returns:	true=strings not equal, false= equal
**************************************************************************/

bool strnccmp(const char *a, const char *b, size_t len)
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
*** Function:	stridcmp
***				Compare property ID strings, i.e. disregarding lowercase characters
*** Parameters: a, b ... strings to be compared
*** Returns:	true=strings not equal, false= equal
**************************************************************************/

bool stridcmp(const char *a, const char *b)
{
	while(*a && *b)
	{
		if(islower(*a)) { a++; continue; }
		if(islower(*b)) { b++; continue; }
		if(*a != *b)
			return true;
		a++;
		b++;
	}

	while(islower(*a))	a++;
	while(islower(*b))	b++;

	if(*a || *b)
		return true;

	return false;
}


/**************************************************************************
*** Function:	strnpcpy
***				Copy string, but replace whitespace with space and
***				and control characters with '.' for safe printing.
***				Destination is _not_ \0 terminated.
*** Parameters: dst ... destination buffer
***				src ... source buffer
***				len ... length
*** Returns:	-
**************************************************************************/

void strnpcpy(char *dst, const char *src, size_t len)
{
	for(; len>0; len--)
	{
		if(isspace(*src))
			*dst = ' ';
		else if(iscntrl(*src))
			*dst = '.';
		else
			*dst = *src;
		dst++;
		src++;
	}
}


/**************************************************************************
*** Function:	KillChars
***				Deletes selected char-set out of a given string
*** Parameters: value ... string
***				len   ... length of string
***				kill  ... type of chars to be removed
***						  C_ISSPACE, C_NOT_ISALPHA or C_NOTinSET/C_inSET
***				cset  ... additional set of chars to be tested
***						  (kill has to be one of C_NOTinSET or C_inSET)
*** Returns:	number of chars removed from string
**************************************************************************/

U_LONG KillChars(char *value, size_t *len, U_SHORT kill, const char *cset)
{
	U_LONG faulty = 0, err = 0;
	size_t i = *len;
	char *c, *d;

	for(c = d = value; i; c++, i--)
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
				if(kill & C_inSET && strchr(cset, *c))
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

	*len -= faulty;
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

U_LONG TestChars(const char *value, U_SHORT test, const char *cset)
{
	U_LONG faulty = 0;
	const char *c;

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
*** Function:	AddProperty
***				Creates new property structure and adds it to the node
*** Parameters: sgfc	... pointer to SGFInfo structure
***				n		... node to which property belongs to
***				id		... tokenized ID of property
***				id_buf	... pointer to property ID string
***				idstr	... ID string
*** Returns:	pointer to new Property structure
***				(exits on fatal error)
**************************************************************************/

struct Property *AddProperty(struct Node *n, token id, U_LONG row, U_LONG col, const char *id_str)
{
	struct Property *newp = SaveMalloc(sizeof(struct Property), "property structure");
	/* init property structure */
	newp->id = id;
	newp->idstr = SaveDupString(id_str, 0, "ID string");
	newp->priority = sgf_token[id].priority;
	newp->flags = sgf_token[id].flags;		/* local copy */
	newp->row = row;
	newp->col = col;
	newp->value = NULL;
	newp->valend = NULL;

	if(n)
		Enqueue(&n->prop, newp);				/* add to node (sorted!) */
	return newp;
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

	free(p->idstr);
	free(p);
	return next;
}

/**************************************************************************
*** Function:	NewNode
***				Inserts a new node into the current SGF tree
*** Parameters: sgfc	  ... pointer to SGFInfo structure
***				parent	  ... parent node
***				row		  ... row number to assign to node
***				col		  ... column number to assign to node
***				new_child ... create a new child for parent node
***							  (insert an empty node into the tree)
*** Returns:	pointer to node or NULL (success / error)
***				(exits on fatal error)
**************************************************************************/

struct Node *NewNode(struct SGFInfo *sgfc, struct Node *parent, U_LONG row, U_LONG col, bool new_child)
{
	struct Node *newn, *hlp;

	newn = SaveMalloc(sizeof(struct Node), "node structure");

	newn->parent	= parent;		/* init node structure */
	newn->child		= NULL;
	newn->sibling	= NULL;
	newn->prop		= NULL;
	newn->last		= NULL;
	newn->row		= row;
	newn->col		= col;

	AddTail(sgfc, newn);

	if(parent)						/* no parent -> root node */
	{
		if(new_child)				/* insert node as new child of parent */
		{
			newn->child = parent->child;
			parent->child = newn;

			hlp = newn->child;		/* set new parent of children */
			while(hlp)
			{
				hlp->parent = newn;
				hlp = hlp->sibling;
			}
		}
		else
		{
			if(!parent->child)			/* parent has no child? */
				parent->child = newn;
			else						/* parent has a child already */
			{							/* -> insert as sibling */
				hlp = parent->child;
				while(hlp->sibling)
					hlp = hlp->sibling;
				hlp->sibling = newn;
			}
		}
	}
	else							/* new root node */
	{
		if(!sgfc->root)				/* first root? */
			sgfc->root = newn;
		else
		{
			hlp = sgfc->root;		/* root sibling */
			while(hlp->sibling)
				hlp = hlp->sibling;
			hlp->sibling = newn;
		}
	}

	return newn;
}


/**************************************************************************
*** Function:	DelNode
***				Deletes empty node *only if*
***				- node has no siblings
***				- has siblings (or is root) but has max. one child
*** Parameters: sgfc  ... pointer to SGFInfo
***				n	  ... node that should be deleted
***				error ... error code to report (while deleting) or E_NO_ERROR
*** Returns:	-
**************************************************************************/

void DelNode(struct SGFInfo *sgfc, struct Node *n, U_LONG error)
{
	struct Node *p, *h;
	struct Property *i;

	p = n->parent;

	/* parent and siblings || root? */
	if((p && (n->sibling || (p->child != n))) || !p)
	{
		/* if child has siblings, deleting would change tree structure */
		if(n->child && n->child->sibling)
			return;
	}

	if(error != E_NO_ERROR)
		PrintError(error, sgfc, n->row, n->col);

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
			if(n->child)
			{
				sgfc->root = n->child;
				sgfc->tree->root = n->child;
				n->child->sibling = n->sibling;
			}
			else					/* delete whole gametree */
			{
				struct TreeInfo *ti = sgfc->tree;
				sgfc->root = n->sibling;
				Delete(&sgfc->tree, ti);
				if(sgfc->info == ti)
					sgfc->info = NULL;
				free(ti);
			}
		}
		else
		{							/* n is subsequent root */
			struct TreeInfo *tiprev = sgfc->tree;
			struct TreeInfo *ti;
			while(tiprev->root->sibling != n)
				tiprev = tiprev->next;
			ti = tiprev->next;
			ti->root = n->child;
			if(n->child)
				tiprev->root->sibling = n->child;
			else					/* delete whole gametree */
			{
				tiprev->root->sibling = n->sibling;
				Delete(&sgfc->tree, ti);
				if(sgfc->info == ti)
					sgfc->info = NULL;
				free(ti);
			}
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

	Delete(&sgfc->first, n);
	free(n);
}


/**************************************************************************
*** Function:	AddPropValue
***				Adds a value to the property (inits structure etc.)
*** Parameters: sgfc	... pointer to SGFInfo structure
***				p		... pointer to property
***				row		... row number associated with property value (or 0)
***				col		... column associated with property value (or 0)
***				value	... pointer to first value
***				size	... length of first value (excluding any 0 bytes)
***				value2	... pointer to second value (or NULL)
***				size2	... length of second value (excluding any 0 bytes)
*** Returns:	pointer to PropValue structure
***				(exits on fatal error)
**************************************************************************/

struct PropValue *AddPropValue(struct SGFInfo *sgfc,
							   struct Property *p, U_LONG row, U_LONG col,
							   const char *value, size_t size,
							   const char *value2, size_t size2)
{
	struct PropValue *newv = SaveMalloc(sizeof(struct PropValue), "property value structure");
	newv->row = row;
	newv->col = col;

	if(value)
	{
		/* +2 because Parse_Float may add 1 char and for trailing '\0' byte */
		newv->value = SaveMalloc(size+2, "property value buffer");
		memcpy(newv->value, value, size);
		*(newv->value + size) = 0;
		newv->value_len = size;
	}
	else
	{
		newv->value = NULL;
		newv->value_len = 0;
	}

	if(value2)
	{
		newv->value2 = SaveMalloc(size2+2, "property value2 buffer");
		memcpy(newv->value2, value2, size2);
		*(newv->value2 + size2) = 0;
		newv->value2_len = size2;
	}
	else
	{
		newv->value2 = NULL;
		newv->value2_len = 0;
	}

	AddTail(&p->value, newv);				/* add value to property */
	return newv;
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
							  const char *value, const char *value2, bool unique)
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
		p = AddProperty(n, id, n->row, n->col, sgf_token[id].id);

	size1 = strlen(value);
	size2 = value2 ? strlen(value2) : 0;
	AddPropValue(sgfc, p, 0, 0, value, size1, value2, size2);

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
*** Function:	CalcGameSig
***				Calculates game signature as proposed by Dave Dyer
*** Parameters: ti 		... first tree info structure
*** 			buffer	... buffer to write game signature into; at least 14 bytes!
*** Returns:	true ... signature calculated // false ... could not calc signature
**************************************************************************/

bool CalcGameSig(struct TreeInfo *ti, char *buffer)
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
