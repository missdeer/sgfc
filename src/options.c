#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	PrintHelp
***				Prints banner + options
**************************************************************************/

void PrintHelp(enum option_help format)
{
	puts(
			" SGFC V1.18  - Smart Game Format Syntax Checker & Converter\n"
			"               Copyright (C) 1996-2018 by Arno Hollosi\n"
			"               Email: <ahollosi@xmp.net>\n"
			" ----------------------------------------------------------");

	if(format == OPTION_HELP_SHORT)
		puts(" 'sgfc -h' for help on options");
	else if (format == OPTION_HELP_LONG)
		puts(" sgfc [options] infile [outfile]\n"
			 " Options:\n"
			 "    -h  ... print this help message\n"
			 "    -bx ... x = 1,2,3: beginning of SGF data is detected by\n"
			 "              1 - sophisticated search algorithm (default)\n"
			 "              2 - first occurrence of '(;'\n"
			 "              3 - first occurrence of '('\n"
			 "    -c  ... write file even if a critical error occurs\n"
			 "    -dn ... n = number : disable message number -n-\n"
			 "    -e  ... expand compressed point lists\n"
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
			 "    -v  ... correct variation level and root moves\n"
			 "    -w  ... disable warning messages\n"
			 "    -yP ... delete property P (P = property id)\n"
			 "    -z  ... reverse ordering of variations"
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
*** Function:	ParseArgs
***				Parses commandline options
***				Options are represented by one char and are preceded with
***				a minus. It's valid to list more than one option per argv[]
*** Parameters: sgfc ... pointer to SGFInfo structure
***				argc ... argument count (like main())
***				argv ... arguments (like main())
*** Returns:	TRUE for success / FALSE in case of argument error
**************************************************************************/

int ParseArgs(struct SGFInfo *sgfc, int argc, char *argv[])
{
	int i, n, m;
	char *c, *hlp;
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
						case 'w':	options->warnings = FALSE;				break;
						case 'u':	options->keep_unknown_props = FALSE;	break;
						case 'o':	options->keep_obsolete_props = FALSE;	break;
						case 'c':	options->write_critical = TRUE;			break;
						case 'e':	options->expand_cpl = TRUE;				break;
						case 'k':	options->keep_head = TRUE;				break;
						case 't':	options->soft_linebreaks = FALSE;		break;
						case 'L':	options->node_linebreaks = TRUE;			break;
						case 'p':	options->pass_tt = TRUE;				break;
						case 's':	options->split_file = TRUE;				break;
						case 'n':	options->del_empty_nodes = TRUE;		break;
						case 'm':	options->del_move_markup = TRUE;		break;
						case 'v':	options->fix_variation = TRUE;			break;
						case 'i':	options->interactive = TRUE;			break;
						case 'g':	options->game_signature = TRUE;			break;
						case 'r':	options->strict_checking = TRUE;		break;
						case 'z':	options->reorder_variations = TRUE;		break;
						case 'h':	options->help = 2;						break;
						case 'd':
							c++; hlp = c;
							n = (int)strtol(c, &c, 10);
							if(n < 1 || n > MAX_ERROR_NUM)
							{
								PrintError(FE_BAD_PARAMETER, sgfc, hlp);
								return(FALSE);
							}
							options->error_enabled[n-1] = FALSE;
							c--;
							break;
						case 'l':
							c++;
							n = *c - '0';
							if(n < 1 || n > 4)
							{
								PrintError(FE_BAD_PARAMETER, sgfc, c);
								return(FALSE);
							}
							options->linebreaks = n;
							break;
						case 'b':
							c++;
							n = *c - '0';
							if(n < 1 || n > 3)
							{
								PrintError(FE_BAD_PARAMETER, sgfc, c);
								return(FALSE);
							}
							options->find_start = n;
							break;
						case 'y':
							c++;
							for(n = 0; isupper(*c); c++, n++);
							c -= n;
							if(n)
							{
								for(m = 1; sgf_token[m].id; m++)
									if(!strnccmp(c, sgf_token[m].id, n))
										break;
							}
							if(!n || !sgf_token[m].id)
							{
								PrintError(FE_BAD_PARAMETER, sgfc, c);
								return(FALSE);
							}
							else
							{
								c += n-1;
								sgf_token[m].flags |= DELETE_PROP;
							}
							break;
						case '-':	/* long options */
							c++;
							if(!strncmp(c, "help", 4))
								options->help = 2;
							else if (!strncmp(c, "version", 7))
								options->help = 1;
							else
							{
								PrintError(FE_UNKNOWN_LONG_OPTION, sgfc, c);
								return(FALSE);
							}
							break;
						default:
						{
							PrintError(FE_UNKNOWN_OPTION, sgfc, *c);
							return(FALSE);
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
					return(FALSE);
				}
				break;
		}
	}

	return(TRUE);
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

	SaveMalloc(struct SGFCOptions *, options, sizeof(struct SGFCOptions), "SGFC options")
	memset(options->error_enabled, TRUE, sizeof(options->error_enabled));
	options->help = FALSE;
	options->warnings = TRUE;
	options->keep_head = FALSE;
	options->keep_unknown_props = TRUE;
	options->keep_obsolete_props = TRUE;
	options->del_empty_nodes = FALSE;
	options->del_move_markup = FALSE;
	options->split_file = FALSE;
	options->write_critical = FALSE;
	options->interactive = FALSE;
	options->linebreaks = OPTION_LINEBREAK_ANY;
	options->soft_linebreaks = TRUE;
	options->node_linebreaks = FALSE;
	options->expand_cpl = FALSE;
	options->pass_tt = FALSE;
	options->fix_variation = FALSE;
	options->find_start = OPTION_FINDSTART_SEARCH;
	options->game_signature = FALSE;
	options->strict_checking = FALSE;
	options->reorder_variations = FALSE;
	options->add_sgfc_ap_property = TRUE;
	options->infile = NULL;
	options->outfile = NULL;
	return options;
}


/**************************************************************************
*** Function:	SetupSGFInfo
***				Allocates SGFInfo structure and initializes it with
***             default values for ->options, ->sfh, and internal structures.
*** Parameters: options ... pointer to SGFCOptions;
***							if NULL filled with SGFCDefaultOptions()
***				sfh     ... SaveFileHandler; if NULL filled with SetupSaveFileIO()
*** Returns:	pointer to SGFInfo structure ready for use in LoadSGF etc.
**************************************************************************/

struct SGFInfo *SetupSGFInfo(struct SGFCOptions *options, struct SaveFileHandler *sfh)
{
	struct SGFInfo *sgfc;
	SaveMalloc(struct SGFInfo *, sgfc, sizeof(struct SGFInfo), "SGFInfo structure")
	memset(sgfc, 0, sizeof(struct SGFInfo));

	if(options)		sgfc->options = options;
	else			sgfc->options = SGFCDefaultOptions();

	if(sfh)			sgfc->sfh = sfh;
	else			sgfc->sfh = SetupSaveFileIO();

	sgfc->_save_c = SetupSaveC_internal();
	sgfc->_util_c = SetupUtilC_internal();
	return sgfc;
}


/**************************************************************************
*** Function:	FreeSGFInfo
***				Frees all memory and other resources of an SGFInfo structure
***				including(!) referenced sub structures and SGFInfo itself
*** Parameters: sgf ... pointer to SGFInfo structure
*** Returns:	-
**************************************************************************/

void FreeSGFInfo(struct SGFInfo *sgf)
{
	struct Node *n, *m;
	struct Property *p;
	struct TreeInfo *t, *hlp;

	if(!sgf)							/* check just to be sure */
		return;

	t = sgf->tree;						/* free TreeInfo's */
	while(t)
	{
		hlp = t->next;
		free(t);
		t = hlp;
	}

	n = sgf->first;						/* free Nodes */
	while(n)
	{
		m = n->next;
		p = n->prop;
		while(p)
			p = DelProperty(n, p);		/* and properties */
		free(n);
		n = m;
	}

	if(sgf->buffer)
		free(sgf->buffer);
	if(sgf->options)
		free(sgf->options);
	if(sgf->sfh)
		free(sgf->sfh);
	if(sgf->_save_c)
		free(sgf->_save_c);
	if(sgf->_util_c)
		free(sgf->_util_c);
	free(sgf);
}
