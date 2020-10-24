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

static void PrintHelp(int everything)
{
	puts(
			" SGFC V1.18  - Smart Game Format Syntax Checker & Converter\n"
			"               Copyright (C) 1996-2018 by Arno Hollosi\n"
			"               Email: <ahollosi@xmp.net>\n"
			" ----------------------------------------------------------");

	if(everything)
		printf("%s%s%s\n",        /* split string for ANSI compliance */
			   " sgfc [options] infile [outfile]\n"
			   " Options:\n"
			   "    -h  ... print this help message\n"
			   "    -bx ... x = 1,2,3: beginning of SGF data is detected by\n"
			   "              1 - sophisticated search algorithm (default)\n"
			   "              2 - first occurence of '(;'\n"
			   "              3 - first occurence of '('\n"
			   "    -c  ... write file even if a critical error occurs\n"
			   "    -dn ... n = number : disable message number -n-\n"
			   "    -e  ... expand compressed point lists\n"
			   "    -g  ... print game signature (Go GM[1] games only)\n",
			   "    -i  ... interactive mode (faulty game-info values only)\n"
			   "    -k  ... keep header in front of SGF data\n"
			   "    -lx ... x = 1,2,3,4: a hard linebreak is\n"
			   "              1 - any linebreak encountered (default)\n"
			   "              2 - any linebreak not preceeded by a space (MGT)\n"
			   "              3 - two linebreaks in a row\n"
			   "              4 - paragraph style (ISHI format, MFGO)\n"
			   "    -L  ... try to keep linebreaks at the end of nodes\n"
			   "    -m  ... delete markup on current move\n",
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
	else
		puts(" 'sgfc -h' for help on options");
}


struct SGFCOptions *SGFCDefaultOptions()
{
	struct SGFCOptions *options;
	SaveMalloc(struct SGFCOptions *, options, sizeof(struct SGFCOptions), "SGFC options")
	options->warnings = TRUE;
	options->keep_head = FALSE;
	options->keep_unknown_props = TRUE;
	options->keep_obsolete_props = TRUE;
	options->del_empty_nodes = FALSE;
	options->del_move_markup = FALSE;
	options->split_file = FALSE;
	options->write_critical = FALSE;
	options->interactive = FALSE;
	options->linebreaks = 1;
	options->softlinebreaks = TRUE;
	options->nodelinebreaks = FALSE;
	options->expandcpl = FALSE;
	options->pass_tt = FALSE;
	options->fix_variation = FALSE;
	options->findstart = 1;
	options->game_signature = FALSE;
	options->strict_checking = FALSE;
	options->reorder_variations = FALSE;
	options->infile = NULL;
	options->outfile = NULL;
	return options;
}


/**************************************************************************
*** Function:	ParseArgs
***				Parses commandline options
***				Options are represented by one char and are preceded with
***				a minus. It's valid to list more than one option per argv[]
*** Parameters: argc ... argument count (like main())
***				argv ... arguments (like main())
*** Returns:	TRUE for ok / FALSE for exit program (help printed)
**************************************************************************/

struct SGFCOptions *ParseArgs(int argc, char *argv[])
{
	int i, n, m;
	char *c, *hlp;
	struct SGFCOptions *options;

	if(argc <= 1)		/* called without arguments */
	{
		PrintHelp(FALSE);
		return(NULL);
	}
	options = SGFCDefaultOptions();

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
						case 'e':	options->expandcpl = TRUE;				break;
						case 'k':	options->keep_head = TRUE;				break;
						case 't':	options->softlinebreaks = FALSE;		break;
						case 'L':	options->nodelinebreaks = TRUE;			break;
						case 'p':	options->pass_tt = TRUE;				break;
						case 's':	options->split_file = TRUE;				break;
						case 'n':	options->del_empty_nodes = TRUE;		break;
						case 'm':	options->del_move_markup = TRUE;		break;
						case 'v':	options->fix_variation = TRUE;			break;
						case 'i':	options->interactive = TRUE;			break;
						case 'g':	options->game_signature = TRUE;			break;
						case 'r':	options->strict_checking = TRUE;		break;
						case 'z':	options->reorder_variations = TRUE;		break;
						case 'd':
							c++; hlp = c;
							n = (int)strtol(c, &c, 10);
							if(n < 1 || n > MAX_ERROR_NUM)
								PrintFatalError(FE_BAD_PARAMETER, NULL, hlp);
							error_enabled[n-1] = FALSE;
							c--;
							break;
						case 'l':
							c++;
							n = *c - '0';
							if(n < 1 || n > 4)
								PrintFatalError(FE_BAD_PARAMETER, NULL, c);
							options->linebreaks = n;
							break;
						case 'b':
							c++;
							n = *c - '0';
							if(n < 1 || n > 3)
								PrintFatalError(FE_BAD_PARAMETER, NULL, c);
							options->findstart = n;
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
								PrintFatalError(FE_BAD_PARAMETER, NULL, c);
							else
							{
								c += n-1;
								sgf_token[m].flags |= DELETE_PROP;
							}
							break;
						case 'h':
							PrintHelp(TRUE);
							free(options);
							return(NULL);
						case '-':	/* long options */
							c++;
							if(!strncmp(c, "help", 4) || !strncmp(c, "version", 7))
							{
								PrintHelp(FALSE);
								free(options);
								return(NULL);
							}
							PrintFatalError(FE_UNKNOWN_LONG_OPTION, NULL, c);
						default:
							PrintFatalError(FE_UNKNOWN_OPTION, NULL, *c);
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
					PrintFatalError(FE_TOO_MANY_FILES, NULL, argv[i]);
				break;
		}
	}

	return(options);
}
