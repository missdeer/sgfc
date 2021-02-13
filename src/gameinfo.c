/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 gameinfo.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	GetFraction
***				Checks for written out fractions and small numbers
*** Parameters: val ... pointer to string
*** Returns:	0 .. nothing found / fraction value (*4)
**************************************************************************/

static int GetFraction(char *val)
{
	int fraction = 0;
	char *t;

	if		((t = strstr(val, "1/2")))	fraction = 2;
	else if ((t = strstr(val, "3/4")))	fraction = 3;
	else if ((t = strstr(val, "1/4")))	fraction = 1;
	else if ((t = strstr(val, "2/4")))	fraction = 2;

	if(t)
		strcpy(t, "   ");		/* remove fraction */

	if(strstr(val, "half"))		fraction = 2;
	if(strstr(val, "one"))		fraction += 4;
	if(strstr(val, "two"))		fraction += 8;
	if(strstr(val, "three"))	fraction += 12;
	if(strstr(val, "four"))		fraction += 16;
	if(strstr(val, "five"))		fraction += 20;

	return fraction;
}


/**************************************************************************
*** Function:	Parse_Komi
***				Checks komi value and corrects it if possible
*** Parameters: value ... pointer to KM string
*** 			len ... length of string
*** Returns:	-1/0/1/2: corrected error / error / ok / corrected
**************************************************************************/

static int Parse_Komi(char *value, size_t *len, ...)
{
	int fraction, ret;
	double points = 0.0;

	fraction = GetFraction(value);

	if((fraction == 4) && strstr(value, "none"))
		fraction = -1;

	ret = Parse_Float(value, len, 0);

	if(fraction)
	{
		if(fraction > 0)
		{
			points = fraction / 4.0;
			if(ret)
				points += atof(value);
		}

		sprintf(value, "%f", points);
		*len = strlen(value);
		Parse_Float(value, len, 0);		/* remove trailing '0' */
		ret = -1;
	}

	return ret;
}


/**************************************************************************
*** Function:	Parse_Time
***				Checks time value and corrects it if possible
*** Parameters: val ... pointer to TM string
***				len ... length of string
*** Returns:	-1/0/1/2: corrected error / error / ok / corrected
**************************************************************************/

static int Parse_Time(char *val, size_t *len, ...)
{
	int ret = 1, hour = 0, min = 0;
	double time;
	char *s;

	if(KillChars(val, len, C_ISSPACE, NULL))
		ret = -1;

	if(!*len)		/* only empty value left -> error */
		return 0;

	/* ":/;+" indicate that there's byo-yomi time given too */
	/* &val[1] because of possible leading '+' */
	if(*len > 1 && TestChars(&val[1], C_inSET, ":/;+"))
		return 0;

	if(TestChars(val, C_ISALPHA, NULL))
	{
		ret = -1;

		s = val + *len - 1;
		if(strstr(val, "hr"))		hour = 3600;
		if(strstr(val, "hour"))		hour = 3600;
		if(*s == 'h')				hour = 3600;
		if(strstr(val, "min"))		min = 60;
		if(*s == 'm')				min = 60;

		if(hour && min)		return 0;		/* can't handle both */
		if(!hour)			hour = min;

		if(Parse_Float(val, len, 0))
		{
			time = atof(val) * hour;
			sprintf(val, "%.1f", time);		/* limit time resolution to 0.1 seconds */
			*len = strlen(val);
			Parse_Float(val, len, 0);		/* remove trailing '0' */
		}
		else
			return 0;
	}
	else
		switch(Parse_Float(val, len, 0))
		{
			case 0:		return 0;
			case -1:	return -1;
			case 2:		if(ret == 1)
							return 2;
		}

	return ret;
}


/**************************************************************************
*** Function:	Parse_Result
***				Checks result value and corrects it if possible
*** Parameters: value ... pointer to RE string
***				len ... length of string
*** Returns:	-1/0/1/2: corrected error / error / ok / corrected
**************************************************************************/

static int Parse_Result(char *value, size_t *len, ...)
{
	char *s, *d;
	int err = 1, charpoints;
	unsigned int type = 0;
	double points = 0.0;

	if(KillChars(value, len, C_ISSPACE, NULL))
		err = -1;

	switch(value[0])
	{
		case '0':
		case '?':	if(*len > 1)
					{
						err = -1;
						value[1] = 0;
						*len = 1;
					}
					break;
		case 'j':
		case 'J':	if(strnccmp(value, "jigo", 4))
						return 0;
		case 'd':	err = -1;
					value[0] = 'D';
		case 'D':	if(!strcmp(value, "Draw"))
						break;
					err = -1;
					strcpy(value, "0");	/* use shortcut for draw */
					*len = 1;
					break;
		case 'v':	err = -1;
					value[0] = 'V';
		case 'V':	if(!strcmp(value, "Void"))
						break;
					err = -1;
					strcpy(value, "Void");
					*len = 4;
					break;
		case 'z':
		case 'Z':	if(strnccmp(value, "zwart", 5))
						return 0;
					value[0] = 'B';
		case 'b':
		case 'w':	err = -1;
					value[0] = (char)toupper(value[0]);
		case 'B':
		case 'W':	charpoints = GetFraction(value);

					if(value[1] != '+')	/* some text between 'B/W' and '+' */
					{
						for(s=value; *s && *s != '+'; s++);
						if(*s)
						{
							err = -1;
							d = &value[1];
							while((*d++ = *s++));  /* copy must be left2right */
							*len = strlen(value);
						}
						else			/* no '+' at all */
						{
							if(strstr(value, "resign")) type |= 1;
							if(strstr(value, "Resign")) type |= 1;
							if(strstr(value, "opgave")) type |= 1;
							if(strstr(value, "win")) type |= 2;
							if(strstr(value, "won")) type |= 2;
							if(strstr(value, "lose")) type |= 4;
							if(strstr(value, "loose")) type |= 4;
							if(strstr(value, "lost")) type |= 4;
							if(strstr(value, "with")) type |= 8;
							if(strstr(value, "by")) type |= 8;
							if(strstr(value, "point")) type |= 16;
							if(strstr(value, "punt")) type |= 16;
							/* type 64: for search for double numbers */

							if((!(type & 7) && !charpoints) ||
								((type & 2) && (type & 4)))	/* win and lose? */
							{
								return 0;
							}

							if(type & 1)	/* resignation */
							{
								if(!(type & 2))
									type |= 4;

								strcpy(&value[1], "+R");
								*len = 3;
							}
							else
								if((type & 24) || charpoints)	/* point win */
								{
									err = Parse_Float_Offset(value, len, 1);

									if(!err && !charpoints)	/* no points found */
									{
										if(type & 16)
											return 0;		/* info would be lost */
										strcpy(&value[1], "+");
										*len = 2;
									}
									else
									{
										if(err)
											points = atof(&value[1]);

										points += (charpoints / 4.0);
										sprintf(&value[1], "+%f", points);
										*len = strlen(value);
										Parse_Float_Offset(value, len, 2);
									}
								}
								else	/* just win or lose */
								{
									strcpy(&value[1], "+");
									*len = 2;
								}

							if(type & 4)
							{
								if(value[0] == 'B')		value[0] = 'W';
								else					value[0] = 'B';
							}
							err = -1;
							break;
						}
					}

					if(value[2])			/* text after the '+' */
					{
						if(!strcmp(&value[2], "Resign"))
							break;
						if(!strcmp(&value[2], "Time"))
							break;
						if(!strcmp(&value[2], "Forfeit"))
							break;
						switch(value[2])	/* looking for shortcuts */
						{				/* or win by points		 */
							case 'r':
							case 't':
							case 'f':	err = -1;
										value[2] = (char)toupper(value[2]);
							case 'R':
							case 'T':
							case 'F':	if(*len > 3)
										{
											err = -1;
											value[3] = 0;
											*len = 3;
										}
										break;

							default:	switch(Parse_Float_Offset(value, len, 2))
										{
											case 0:		err = 0;	break;
											case -1:	err = -1;	break;
											case 1:		break;
											case 2:		if(err == 1)
															err = 2;
														break;
										}

										if(charpoints)
										{
											err = -1;
											points = atof(&value[2]) + (charpoints / 4.0);
											sprintf(&value[2], "%f", points);
											*len = strlen(value);
											Parse_Float_Offset(value, len, 2);
										}
										else
											if(!err)
											{
												value[2] = 0;	/* win without reason */
												*len = 2;
												err = -1;
											}
										break;
						}
					}
					break;

		default:	err = 0;
					break;
	}

	return err;
}


/**************************************************************************
*** Function:	CorrectDate
***				Tries to fix date value
*** Parameters: value ... pointer to date string
***				len ... length of string
*** Returns:	-1/0: corrected error / error
**************************************************************************/

static int CorrectDate(char *value, size_t *len)
{
	long year = -1, month = -1, day = -1, day2 = -1;
	int i;
	bool char_month = false;
	long n;
	char *s;
	const char months[26][4] = { "Jan", "jan", "Feb", "feb", "Mar", "mar",
								"Apr", "apr", "May", "may", "Jun", "jun",
								"Jul", "jul", "Aug", "aug", "Sep", "sep",
								"Oct", "oct", "Nov", "nov", "Dec", "dec",
								"Okt", "okt" };

	KillChars(value, len, C_inSET, "\n");

	for(i = 0; i < 26; i++)
	{
		s = strstr(value, months[i]);
		if(s)
		{
			if(char_month)		/* found TWO month names */
				return 0;
			month = i/2 + 1;
			char_month = true;
		}
	}

	if(month == 13)		month = 10;		/* correct 'okt' value */

	s = value;
	while(*s)
	{
		if(isdigit(*s))
		{
			n = strtol(s, &s, 10);

			if(n > 31)
				if(year < 0)	year = n;
				else			return 0;	/* two values >31 */
			else
			if(n > 12 || char_month)
				if(day < 0)		day = n;
				else
				if(day2 < 0)	day2 = n;
				else			return 0;	/* more than two days found */
			else
				if(month < 0)	month = n;
				else			return 0;	/* can't tell if MM or DD */
		}
		else
			s++;
	}

	if(year < 0 || year > 9999)	/* year is missing or false */
		return 0;

	if(year < 100)				/* only two digits? -> 20th century */
		year += 1900;

	if(day > 0 && month > 0)
	{
		if(day2 > 0)
			sprintf(value, "%04ld-%02ld-%02ld,%02ld", year, month, day, day2);
		else
			sprintf(value, "%04ld-%02ld-%02ld", year, month, day);
	}
	else
		if(month > 0)
			sprintf(value, "%04ld-%02ld", year, month);
		else
			sprintf(value, "%04ld", year);

	*len = strlen(value);
	return -1;
}


/**************************************************************************
*** Function:	Parse_Date
***				Checks date value, corrects easy errors (space etc.)
***				Tough cases are passed on to CorrectDate
*** Parameters: value ... pointer to date string
***				len	  ... length of string
*** Returns:	-1/0/1: corrected error / error / ok
**************************************************************************/

static int Parse_Date(char *value, size_t *len, ...)
{
	int ret = 1, allowed, type, turn, oldtype;
	bool has_year;
	char *c, *d;
	long num;
	/* type:	0 YYYY
				1 YYYY-MM
				2 YYYY-MM-DD
				3 MM-DD
				4 MM
				5 DD
	   allowed: bitmask of type
	*/

	/* bad chars? -> pass on to CorrectDate */
	if(TestChars(value, C_NOTinSET, "0123456789-,"))
		return CorrectDate(value, len);

	c = d = value;
	while(*c)				/* remove spaces, and unnecessary '-', ',' */
	{
		if(isspace(*c))		/* keep spaces in between numbers */
		{
			if(ret)	ret = -1;
			if((d != value) && isdigit(*(d-1)) && isdigit(*(c+1)))
			{
				*d++ = *c++;	/* space between two numbers */
				ret = 0;
			}
			else
				c++;
		}
		else
		if(*c == '-')		/* remove all '-' not in between two numbers */
		{
			if((d != value) && isdigit(*(d-1)) && (isdigit(*(c+1)) || isspace(*(c+1))))
				*d++ = *c++;
			else
			{
				if(ret)	ret = -1;
				c++;
			}
		}
		else
		if(*c == ',')		/* remove all ',' not preceeded by a number */
		{
			if((d != value) && isdigit(*(d-1)) && *(c+1))
				*d++ = *c++;
			else
			{
				if(ret)	ret = -1;
				c++;
			}
		}
		else
			*d++ = *c++;
	}
	*d = 0;


	c = value;
	allowed = 0x07;
	oldtype = type = 0;
	has_year = false;
	turn = 1;

	while(ret && *c)			/* parse the nasty bastard */
	{
		d = c;
		num = strtol(c, &c, 10);
		if((num < 1) || (num > 9999) || (((c-d) != 2) && ((c-d) != 4)) ||
			(turn == 1 && ((c-d) == 2) && !(allowed & 0x30)) ||
			(turn != 1 && ((c-d) == 4)))
		{
			/* illegal number or != 2 or 4 digits
			   or start number isn't year when required
			   or digits inside date (MM,DD part) */
			ret = 0;
			break;
		}

		if((c-d) == 4)			/* date has year */
			has_year = true;

		switch(*c)
		{
			case '-':	if(((c-d) == 2) && num > 31)
							ret = 0;
						c++;			/* loop inc */
						turn++;
						if(turn == 4)
							ret = 0;
						break;

			case ',':	c++;			/* loop inc */

			case 0:		switch(turn)	/* date has got which type? */
						{
							case 1: if(has_year) type = 0;
									else
										if(oldtype == 1 || oldtype == 4)
											type = 4;
										else
											type = 5;
									break;
							case 2: if(has_year) type = 1;
									else		 type = 3;
									break;
							case 3:	type = 2;
									break;
						}

						if(!(allowed & (1 << type)))
						{
							ret = 0;	/* is current type allowed? */
							break;
						}

						switch(type)	/* set new allow mask */
						{
							case 0:	allowed = 0x07;	break;
							case 1:	allowed = 0x1f;	break;
							case 2:	allowed = 0x2f;	break;
							case 3:	allowed = 0x2f;	break;
							case 4:	allowed = 0x1f;	break;
							case 5:	allowed = 0x2f;	break;
						}

						turn = 1;		/* new date to parse */
						has_year = false;
						oldtype = type;
						break;
		}
	}

	*len = strlen(value);
	if(!ret)		/* date has got tough errors -> pass on to CorrectDate */
		ret = CorrectDate(value, len);

	return ret;
}


/**************************************************************************
*** Function:	PromptGameInfo
***				If interactive mode: prompts for game-info value
***				else just print error message
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... property
***				v	 ... faulty value (part of p)
*** Returns:	true / false if property should be deleted
**************************************************************************/

static int PromptGameInfo(struct SGFInfo *sgfc, struct Property *p,
		struct PropValue *v, int (*Parse_Value)(char *, size_t *, ...))
{
	char *newgi, inp[2001];
	size_t size;
	int ret;

	if(!sgfc->options->interactive)
	{
		PrintError(E4_FAULTY_GC, sgfc, v->row, v->col, v->value, p->idstr, "(not corrected!)");
		return true;
	}

	size = v->value_len;
	if(size < 25)		/* CorrectDate may use up to 15 chars */
		size = 25;
	newgi = SaveDupString(v->value, size, "game info value buffer");

	PrintError(E4_FAULTY_GC, sgfc, v->row, v->col, v->value, p->idstr, "");

	while(true)
	{
		ret = (*Parse_Value)(newgi, 0, sgfc);

		if(ret)	printf("--> Use [%s] (enter), delete (d) or type in new value? ", newgi);
		else	printf("--> Keep faulty value (enter), delete (d) or type in new value? ");

		fgets(inp, 2000, stdin);
		if(strlen(inp))
			inp[strlen(inp)-1] = 0;	/* delete last char, it is a newline */

		if(!strnccmp(inp, "d", 0))	/* delete */
		{
			free(newgi);
			return false;
		}

		if(strlen(inp))			/* edit */
		{
			ret = (*Parse_Value)(inp, 0);
			if(ret == 1)
			{
				free(v->value);
				v->value_len = strlen(inp);
				v->value = SaveMalloc(v->value_len+4, "game info value buffer");
				strcpy(v->value, inp);
				break;
			}

			puts("--! Error in input string !--");
			if(ret == -1)
			{
				size = (strlen(inp) > 25) ? strlen(inp) : 25;
				free(newgi);
				newgi = SaveDupString(inp, size, "game info value buffer");
			}
		}
		else					/* [return] */
			break;
	}

	free(newgi);
	return true;
}


/**************************************************************************
*** Function:	Check_GameInfo
***				Checks RE,DT,TM, KM value
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_GameInfo(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	char *val;
	size_t size;
	int res;
	int (*parse)(char *, size_t *, ...);

	if(!Check_Text(sgfc, p, v))		/* parse text (converts spaces) */
		return false;

	switch(p->id)
	{
		case TKN_RE:	parse = Parse_Result;		break;
		case TKN_DT:	parse = Parse_Date;			break;
		case TKN_TM:	parse = Parse_Time;			break;
		case TKN_KM:	parse = Parse_Komi;			break;
		default:		return true;
	}

	size = (v->value_len > 25-8) ? (v->value_len + 8) : (25+1);
	/* correct functions may use up to 25 bytes; +8 because time in hours multiplies by 3600 and adds ".0" */

	val = SaveMalloc(size, "result value buffer");
	strcpy(val, v->value);
	size_t val_len = v->value_len;
	res = (*parse)(val, &val_len);

	if(sgfc->options->interactive)
	{
		if(res < 1 && !PromptGameInfo(sgfc, p, v, parse))
		{
			free(val);
			return false;
		}
	}
	else
	{
		switch(res)
		{
			case 0:
				PrintError(E4_FAULTY_GC, sgfc, v->row, v->col, v->value, p->idstr, "(NOT CORRECTED!)");
				break;
			case -1:
				PrintError(E4_BAD_VALUE_CORRECTED, sgfc, v->row, v->col, v->value, p->idstr, val);
				free(v->value);
				v->value = val;
				v->value_len = val_len;
				return true;
		}
	}

	if(res == 2)
	{
		strcpy(v->value, val);
		v->value_len = val_len;
	}

	free(val);
	return true;
}
