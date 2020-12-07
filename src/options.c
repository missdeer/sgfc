/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 options.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	PrintHelp
***				Prints banner + options
**************************************************************************/

void PrintHelp(const enum option_help format)
{
	puts(" SGFC v1.18  - Smart Game Format Syntax Checker & Converter");
	if(format == OPTION_HELP_VERSION)
		return;

	puts("               Copyright (C) 1996-2018 by Arno Hollosi\n"
		 "               Email: <ahollosi@xmp.net>\n"
		 " ----------------------------------------------------------");

	if(format == OPTION_HELP_SHORT)
		puts(" 'sgfc -h' for help on options");
	else if (format == OPTION_HELP_LONG)
		puts(" sgfc [options] infile [outfile]\n\n"
			 " Options:\n"
			 "    -h  ... print this help message\n"
			 "    -bx ... x = 1,2,3: beginning of SGF data is detected by\n"
			 "              1 - smart search algorithm (default)\n"
			 "              2 - first occurrence of '(;'\n"
			 "              3 - first occurrence of '('\n"
			 "    -c  ... write file even if a critical error occurs\n"
			 "    -dn ... n = number : disable message number -n-\n"
			 "    -e  ... expand compressed point lists\n"
			 "    -Ex ... x = 1,2,3: charset encoding is applied to\n"
			 "              1 - to whole SGF file, _before_ parsing (unit=char; default)\n"
			 "              2 - text property values only, _after_ parsing (unit=byte)\n"
			 "              3 - no encoding applied (binary style; unit=byte)\n"
			 "    -g  ... print game signature (Go GM[1] games only)\n"
			 "    -i  ... interactive mode (faulty game-info values only)\n"
			 "    -k  ... keep header in front of SGF data\n"
			 "    -lx ... x = 1,2,3,4: a hard linebreak is\n"
			 "              1 - any linebreak encountered (default)\n"
			 "              2 - any linebreak not preceded by a space (MGT)\n"
			 "              3 - two linebreaks in a row\n"
			 "              4 - paragraph style (ISHI format, MFGO)\n"
			 "    -L  ... try to keep linebreaks at the end of nodes\n"
			 "    -m  ... delete markup on current move\n"
			 "    -n  ... delete empty nodes\n"
			 "    -o  ... delete obsolete properties\n"
			 "    -p  ... write pass moves as '[tt]' if possible\n"
			 "    -r  ... restrictive checking\n"
			 "    -s  ... split game collection into single files\n"
			 "    -t  ... do not insert any soft linebreaks into text values\n"
			 "    -u  ... delete unknown properties\n"
			 "    -U  ... alias for '--default-encoding=UTF-8'\n"
			 "    -v  ... correct variation level and root moves\n"
			 "    -w  ... disable warning messages\n"
			 "    -yP ... delete property P (P = property id)\n"
			 "    -z  ... reverse ordering of variations\n\n"
			 "    --default-encoding=name ... set default encoding to 'name' (CA[] has priority)\n"
			 "    --encoding=name         ... override encoding specified in SGF file with 'name'\n"
			 "    --help    ... print long help text (same as -h)\n"
			 "    --version ... print version only\n"
		);
}


/**************************************************************************
*** Function:	PrintStatusLine
***				Prints final status line with error/warning count
*** Parameters: sgfc ... pointer to SGFInfo
*** Returns:	-
**************************************************************************/

void PrintStatusLine(const struct SGFInfo *sgfc) {
	printf("%s: ", sgfc->options->infile);

	if(sgfc->error_count || sgfc->warning_count)	/* errors & warnings */
	{
		if(sgfc->error_count)
			printf("%d error(s)  ", sgfc->error_count);

		if(sgfc->warning_count)
			printf("%d warning(s)  ", sgfc->warning_count);

		if(sgfc->critical_count)
			printf("(critical:%d)  ", sgfc->critical_count);
	}
	else								/* file ok */
		printf("OK  ");

	if(sgfc->ignored_count)
		printf("(%d message(s) ignored)", sgfc->ignored_count);

	printf("\n");
}


/**************************************************************************
*** Function:	PrintGameSignatures
***				Prints game signatures of all game trees to stdout.
*** Parameters: sgfc ... pointer to SGFInfo structure
*** Returns:	true for success / false in case of argument error
**************************************************************************/

void PrintGameSignatures(const struct SGFInfo *sgfc)
{
	struct TreeInfo *ti;
	char signature[14];

	ti = sgfc->tree;
	while(ti)
	{
		if(CalcGameSig(ti, signature))
			printf("Game signature - tree %d: '%s'\n", ti->num, signature);
		else
			printf("Game signature - tree %d: contains GM[%d] "
				   "- can't calculate signature\n", ti->num, ti->GM);
		ti = ti->next;
	}
}


/**************************************************************************
*** Function:	ParsePropertyArg
***				Helper function for ParseArgs(): reads in property name
***				(uppercase letters) and returns property ID
*** Parameters: sgfc ... pointer to SGFInfo structure
***				str  ... pointer to current position in argument
*** Returns:	property ID or -1 in case of error
**************************************************************************/

static int ParsePropertyArg(struct SGFInfo *sgfc, const char **str)
{
	const char *c = *str;
	int n, m;

	c++; /* first char after initial option letter */
	/* count uppercase */
	for(n = 0; isupper(*c); c++, n++);
	c -= n;
	if(n)
	{
		/* check for known property name */
		for(m = 1; sgf_token[m].id; m++)
			if(!strnccmp(c, sgf_token[m].id, n))
				break;
	}
	/* no property specified or unknown property -> error */
	if(!n || !sgf_token[m].id)
	{
		PrintError(FE_BAD_PARAMETER, sgfc, c);
		return -1;
	}
	*str = c + n - 1;
	return m;
}


/**************************************************************************
*** Function:	ParseIntArg
***				Helper function for ParseArgs(): reads in integer value
*** Parameters: sgfc ... pointer to SGFInfo structure
***				str  ... pointer to current position in argument
***				max  ... maximum allowed number
*** Returns:	integer in range 1..max or 0 in case of error
**************************************************************************/

static int ParseIntArg(struct SGFInfo *sgfc, const char **str, int max)
{
	char *hlp;
	int n;

	n = (int)strtol(*str+1, &hlp, 10);
	if(n < 1 || n > max)
	{
		PrintError(FE_BAD_PARAMETER, sgfc, *str+1);
		return 0;
	}
	*str = hlp - 1;
	return n;
}


/**************************************************************************
*** Function:	ValidateEncoding
***				Helper function for ParseArgs(): checks if encoding is known to iconv
*** Parameters: sgfc	 ... pointer to SGFInfo structure
***				encoding ... name of encoding
***				argname	 ... context for error message
*** Returns:	*encoding or NULL
**************************************************************************/

static const char *ValidateEncoding(struct SGFInfo *sgfc, const char *encoding, const char *argname)
{
	iconv_t test = iconv_open("UTF-8", encoding);
	if(test == (iconv_t)-1)
	{
		PrintError(FE_UNKNOWN_ENCODING, sgfc, argname, encoding);
		return NULL;
	}
	iconv_close(test);
	return encoding;
}

/**************************************************************************
*** Function:	ParseArgs
***				Parses commandline options
***				Options are represented by one char and are preceded with
***				a minus. It's valid to list more than one option per argv[]
*** Parameters: sgfc ... pointer to SGFInfo structure
***				argc ... argument count (like main())
***				argv ... arguments (like main())
*** Returns:	true for success / false in case of argument error
**************************************************************************/

bool ParseArgs(struct SGFInfo *sgfc, int argc, const char *argv[])
{
	int i, n;
	const char *c;
	struct SGFCOptions *options = sgfc->options;

	for(i = 1; i < argc; i++)
	{
		switch(argv[i][0])
		{
			case '-':
				for(c = &argv[i][1]; *c; c++)
				{
					switch(*c)
					{
						case 'w':	options->warnings = false;				break;
						case 'u':	options->keep_unknown_props = false;	break;
						case 'o':	options->keep_obsolete_props = false;	break;
						case 'c':	options->write_critical = true;			break;
						case 'e':	options->expand_cpl = true;				break;
						case 'k':	options->keep_head = true;				break;
						case 't':	options->soft_linebreaks = false;		break;
						case 'L':	options->node_linebreaks = true;		break;
						case 'p':	options->pass_tt = true;				break;
						case 's':	options->split_file = true;				break;
						case 'n':	options->del_empty_nodes = true;		break;
						case 'm':	options->del_move_markup = true;		break;
						case 'v':	options->fix_variation = true;			break;
						case 'i':	options->interactive = true;			break;
						case 'g':	options->game_signature = true;			break;
						case 'r':	options->strict_checking = true;		break;
						case 'z':	options->reorder_variations = true;		break;
						case 'h':	options->help = OPTION_HELP_LONG;		break;
						case 'U':	options->default_encoding = "UTF-8";	break;
						case 'd':
							if(!(n = ParseIntArg(sgfc, &c, MAX_ERROR_NUM)))
								return false;
							options->error_enabled[n-1] = false;
							break;
						case 'l':
							if(!(n = ParseIntArg(sgfc, &c, 4)))
								return false;
							options->linebreaks = n;
							break;
						case 'b':
							if(!(n = ParseIntArg(sgfc, &c, 3)))
								return false;
							options->find_start = n;
							break;
						case 'E':
							if(!(n = ParseIntArg(sgfc, &c, 3)))
								return false;
							options->encoding = n;
							break;
						case 'y':
							if((n = ParsePropertyArg(sgfc, &c)) == -1)
								return false;
							options->delete_property[n] = true;
							break;
						case '-':	/* long options */
							c++;
							if(!strcmp(c, "help"))
								options->help = OPTION_HELP_LONG;
							else if(!strcmp(c, "version"))
								options->help = OPTION_HELP_VERSION;
							else if(!strncmp(c, "encoding=", 9))
							{
								options->forced_encoding = ValidateEncoding(sgfc, &argv[i][9+2], "encoding");
								if(!options->forced_encoding)
									return false;
							}
							else if(!strncmp(c, "default-encoding=", 17))
							{
								options->default_encoding = ValidateEncoding(sgfc, &argv[i][17+2], "default-encoding");
								if(!options->default_encoding)
									return false;
							}
							else
							{
								PrintError(FE_UNKNOWN_LONG_OPTION, sgfc, c);
								return false;
							}
							goto argument_parsed;
						default:
						{
							PrintError(FE_UNKNOWN_OPTION, sgfc, *c);
							return false;
						}
					}
				}
				break;

			default:			/* argument isn't preceded by '-' */
				if(!options->infile)
					options->infile = argv[i];
				else
				if(!options->outfile)
					options->outfile = argv[i];
				else
				{
					PrintError(FE_TOO_MANY_FILES, sgfc, argv[i]);
					return false;
				}
				break;
		}
argument_parsed:;
	}

	return true;
}


/**************************************************************************
*** Function:	SGFCDefaultOptions
***				Allocates SGFCOptions structure and initializes it with
***				default option values.
*** Parameters: -
*** Returns:	-
**************************************************************************/

struct SGFCOptions *SGFCDefaultOptions(void)
{
	struct SGFCOptions *options;

	options = SaveMalloc(sizeof(struct SGFCOptions), "SGFC options");
	memset(options->error_enabled, true, sizeof(options->error_enabled));
	memset(options->delete_property, false, sizeof(options->delete_property));
	options->help = OPTION_HELP_NONE;
	options->warnings = true;
	options->keep_head = false;
	options->keep_unknown_props = true;
	options->keep_obsolete_props = true;
	options->del_empty_nodes = false;
	options->del_move_markup = false;
	options->split_file = false;
	options->write_critical = false;
	options->interactive = false;
	options->linebreaks = OPTION_LINEBREAK_ANY;
	options->soft_linebreaks = true;
	options->node_linebreaks = false;
	options->expand_cpl = false;
	options->pass_tt = false;
	options->fix_variation = false;
	options->find_start = OPTION_FINDSTART_SEARCH;
	options->game_signature = false;
	options->strict_checking = false;
	options->reorder_variations = false;
	options->add_sgfc_ap_property = true;
	options->encoding = OPTION_ENCODING_EVERYTHING;
	options->infile = NULL;
	options->outfile = NULL;
	options->forced_encoding = NULL;
	options->default_encoding = "ISO-8859-1"; /* according to SGF spec */
	return options;
}


/**************************************************************************
*** Function:	SetupSGFInfo
***				Allocates SGFInfo structure and initializes it with
***             default values for ->options, ->sfh, and internal structures.
*** Parameters: options ... pointer to SGFCOptions;
***							if NULL filled with SGFCDefaultOptions()
*** Returns:	pointer to SGFInfo structure ready for use in LoadSGF etc.
**************************************************************************/

struct SGFInfo *SetupSGFInfo(struct SGFCOptions *options)
{
	struct SGFInfo *sgfc = SaveCalloc(sizeof(struct SGFInfo), "SGFInfo structure");

	if(options)		sgfc->options = options;
	else			sgfc->options = SGFCDefaultOptions();

	sgfc->_error_c = SetupErrorC_internal();
	return sgfc;
}


/**************************************************************************
*** Function:	FreeSGFInfo
***				Frees all memory and other resources of an SGFInfo structure
***				including(!) referenced sub structures and SGFInfo itself
*** Parameters: sgfc ... pointer to SGFInfo structure
*** Returns:	-
**************************************************************************/

void FreeSGFInfo(struct SGFInfo *sgfc)
{
	struct Node *n, *m;
	struct Property *p;
	struct TreeInfo *t, *hlp;

	if(!sgfc)							/* check just to be sure */
		return;

	t = sgfc->tree;						/* free TreeInfo's */
	while(t)
	{
		if(t->encoding)
			iconv_close(t->encoding);
		hlp = t->next;
		free(t);
		t = hlp;
	}

	n = sgfc->first;						/* free Nodes */
	while(n)
	{
		m = n->next;
		p = n->prop;
		while(p)
			p = DelProperty(n, p);		/* and properties */
		free(n);
		n = m;
	}

	if(sgfc->global_encoding_name)
		free(sgfc->global_encoding_name);
	if(sgfc->buffer)
		free(sgfc->buffer);
	if(sgfc->options)
		free(sgfc->options);
	if(sgfc->_error_c)
		free(sgfc->_error_c);
	free(sgfc);
}
