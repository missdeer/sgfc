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
#include <stdlib.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	main
*** Parameters: as usual
*** Returns:	 0 on success
***				 5 if there were warnings
***				10 if there were errors
***				20 if a fatal error occurred
**************************************************************************/

#ifndef VERSION_NO_MAIN

int main(int argc, char *argv[])
{
	int ret = 0;
	struct SGFInfo *sgfc;
	struct SGFCOptions *options;

	options = ParseArgs(argc, argv);

	if(!options)	/* no options given? */
	{
		PrintHelp(FALSE);
		return(0);
	}
	if(options->help)
	{
		PrintHelp(options->help);
		free(options);
		return(0);
	}

	if(!options->infile)
		PrintFatalError(FE_MISSING_SOURCE_FILE, NULL);

	sgfc = Setup_SGFInfo(options, NULL);

	LoadSGF(sgfc, options->infile);
	ParseSGF(sgfc);

	if(options->outfile)
	{
		if(options->write_critical || !sgfc->critical_count)
			SaveSGF(sgfc, options->outfile);
		else
			PrintError(E_CRITICAL_NOT_SAVED, NULL);
	}

	if(sgfc->error_count)			ret = 10;
	else if (sgfc->warning_count)	ret = 5;

	PrintStatusLine(sgfc);
	FreeSGFInfo(sgfc);

	return(ret);
}
#endif
