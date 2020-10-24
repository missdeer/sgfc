/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 main.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
***
*** Copyright notice:
***
*** This program is open source software; you can redistribute it
*** and/or modify it under the terms of the BSD License (see file COPYING)
***
*** This program is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
***
*** The author can be reached at <ahollosi@xmp.net>
***
**************************************************************************/

#include <stdio.h>
#include <string.h>

#include "all.h"
#include "protos.h"

static struct SGFInfo sgf;


/**************************************************************************
*** Function:	main
*** Parameters: as usual
*** Returns:	 0 on success
***				 5 if there were warnings
***				10 if there were errors
***				20 if a fatal error occured
**************************************************************************/

#ifndef VERSION_NO_MAIN

int main(int argc, char *argv[])
{
	int ret = 0;
	struct SGFCOptions *options;

	memset(error_enabled, TRUE, sizeof(error_enabled));

	options = ParseArgs(argc, argv);
	if(!options)	/* help printed? */
		return(0);
	if(!options->infile)
		PrintFatalError(FE_MISSING_SOURCE_FILE, NULL);

	memset(&sgf, 0, sizeof(struct SGFInfo));	/* init SGFInfo structure */
	sgf.options = options;

	LoadSGF(&sgf, options->infile);
	ParseSGF(&sgf);

	if(options->outfile)
	{
		if(options->write_critical || !critical_count)
		{
			sgf.sfh = &save_file_io;
			SaveSGF(&sgf, options->outfile);
		}
		else
			PrintError(E_CRITICAL_NOT_SAVED, NULL);
	}

	fprintf(E_OUTPUT, "%s: ", options->infile);	/* print status line */

	if(error_count || warning_count)			/* errors & warnings */
	{
		ret = 5;
		if(error_count)
		{
			fprintf(E_OUTPUT, "%d error(s)  ", error_count);
			ret = 10;
		}

		if(warning_count)
			fprintf(E_OUTPUT, "%d warning(s)  ", warning_count);

		if(critical_count)						/* critical ones */
			fprintf(E_OUTPUT, "(critical:%d)  ", critical_count);
	}
	else										/* file ok */
		fprintf(E_OUTPUT, "OK  ");

	if(ignored_count)
		fprintf(E_OUTPUT, "(%d message(s) ignored)", ignored_count);

	fprintf(E_OUTPUT, "\n");

	FreeSGFInfo(&sgf);
	return(ret);
}
#endif
