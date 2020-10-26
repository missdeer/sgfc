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
	int ret = 20;
	struct SGFInfo *sgfc;

	if(argc <= 1)		/* called without arguments */
	{
		PrintHelp(OPTION_HELP_SHORT);
		return(0);
	}

	sgfc = SetupSGFInfo(NULL, NULL);

	if(!ParseArgs(sgfc, argc, argv))
		goto fatal_error;

	if(sgfc->options->help)
	{
		PrintHelp(sgfc->options->help);
		FreeSGFInfo(sgfc);
		return(0);
	}

	if(!sgfc->options->infile)
	{
		PrintError(FE_MISSING_SOURCE_FILE, sgfc);
		goto fatal_error;
	}

	if(!LoadSGF(sgfc, sgfc->options->infile))
		goto fatal_error;

	ParseSGF(sgfc);

	if(sgfc->options->outfile)
	{
		if(sgfc->options->write_critical || !sgfc->critical_count)
			SaveSGF(sgfc, sgfc->options->outfile);
		else
			PrintError(E_CRITICAL_NOT_SAVED, sgfc);
	}

	if(sgfc->error_count)			ret = 10;
	else if (sgfc->warning_count)	ret = 5;
	else							ret = 0;

	PrintStatusLine(sgfc);

fatal_error:
	FreeSGFInfo(sgfc);
	return(ret);
}
#endif
