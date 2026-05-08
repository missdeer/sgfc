/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 parse.c
***
*** Copyright (C) 1996-2026 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "all.h"
#include "protos.h"
#include "helpers.h"


/**************************************************************************
*** Function:	ParseText_Unescape - helper function for Parse_Text
***				Unescape text values, i.e. handling of '\'
*** Parameters: s	... pointer to property value
***				len	... length of string
*** Returns:	-
**************************************************************************/

static void ParseText_Unescape(char *s, size_t *len)
{
	char *end = s + *len;
	char *d = s;                        /* remove unnecessary '\' */

	while(s < end)
	{
		if(*s != '\\')
		{
			*d++ = *s++;
			continue;
		}

		if(*(s+1) != '\n' && *(s+1) != '\r')
		{
			s++;
			(*len)--;
			*d++ = *s++;
			continue;
		}

		/* soft linebreak */
		s += 2;
		(*len) -= 2;
		/* CRLF or LFCR */
		if(s < end && (*s == '\n' || *s == '\r') && *s != *(s-1))
		{
			s++;
			(*len)--;
		}
	}
	*d = 0;
}


/**************************************************************************
*** Function:	ParseText_Decode - helper function for Parse_Text
***				Decoding in case of OPTION_ENCODING_TEXT_ONLY
*** Parameters: value_ptr ... pointer to property value
***				len		  ... length of string
*** Returns:	true on success, false on fatal encoding error
**************************************************************************/

static bool ParseText_Decode(struct SGFInfo *sgfc, char **value_ptr, size_t *len)
{
	const char *end;
	char *decoded = DecodeBuffer(sgfc, sgfc->info->encoding, *value_ptr, *len, 0, &end);
	if(!decoded)
	{
		**value_ptr = 0;		/* in case of error: delete property value */
		*len = 0;
		return false;
	}

	free(*value_ptr); 			/* swap buffer for decoded buffer */
	*value_ptr = decoded;
	*len = (size_t)(end - decoded);
	return true;
}


/**************************************************************************
*** Function:	ParseText_NormalizeWhitespace - helper function for Parse_Text
***				Normalize whitespace and linebreaks; replace $00 bytes with space
*** Parameters: s	... pointer to property value
***				len		... length of string
***				row		... row number of property value
***				col		... column of property value
*** Returns:	-
**************************************************************************/

static void ParseText_NormalizeWhitespace(struct SGFInfo *sgfc, char *s, size_t *len, uint32_t row, uint32_t col)
{
	char old = 0;
	char *d = s;
	char *end = s + *len;

	while(s < end)						/* transform linebreaks to '\n' */
	{									/*			and all WS to space */
		if(*s == '\r' || *s == '\n')	/* linebreak char? */
		{
			if(old && old != *s)		/* different from preceding char? */
			{
				(*len)--;
				old = 0;				/* -> no real linebreak */
			}
			else
			{
				old = *s;
				*d++ = '\n';			/* insert linebreak */
			}
		}
		else							/* other chars than \r,\n */
		{
			old = 0;
			if(ch_isspace(*s))				/* transform all WS to space */
				*d++ = ' ';
			else if(!*s)				/* replace \0 bytes with space, so that we can use NULL terminated strings */
			{
				PrintError(W_CTRL_BYTE_DELETED, sgfc, row, col+1);
				*d++ = ' ';
			}
			else
				*d++ = *s;
		}
		s++;
	}
	*d = 0;
}


/**************************************************************************
*** Function:	ParseText_ApplyLinebreakStyle - helper function for Parse_Text
***				Applies linebreak style according to specified options
*** Parameters: value	... pointer to property value
***				len		... length of string
***				flags	... property flags for detecting SimpleText values
*** Returns:	-
**************************************************************************/

static void ParseText_ApplyLinebreakStyle(struct SGFInfo *sgfc, char *value, size_t *len, uint16_t flags)
{
	char *end = value + *len;
	char *d = value, *s = value;

	while(s < end)
	{
		if(*s != '\n')
		{
			*d++ = *s++;
			continue;
		}
		if (flags & PVT_SIMPLE)
		{
			*d++ = ' ';
			s++;
			continue;
		}

		switch(sgfc->options->linebreaks)
		{
			case OPTION_LINEBREAK_ANY:	/* every line break encountered */
				*d++ = *s++;
				break;
			case OPTION_LINEBREAK_NOSPACE:	/* MGT style */
				if((s != value) && (*(s-1) == ' '))
				{
					*d++ = ' ';
					s++;
				}
				else
					*d++ = *s++;
				break;
			case OPTION_LINEBREAK_2BRK: /* two linebreaks in a row */
				if(*(s+1) == '\n')
				{
					*d++ = *s;
					s += 2;
					(*len)--;
				}
				else
				{
					*d++ = ' ';
					s++;
				}
				break;
			case OPTION_LINEBREAK_PRGRPH: /* paragraph style (ISHI format, MFGO) */
				if(*(s+1) == '\n')
				{
					*d++ = *s++;
					*d++ = *s++;
				}
				else
				{
					*d++ = ' ';
					s++;
				}
				break;
		}
	}
	*d = 0;
}


/**************************************************************************
*** Function:	ParseText_StripTrailingSpace - helper function for Parse_Text
***				Strips trailing whitespace
*** Parameters: value	... pointer to property value
***				len		... length of string
*** Returns:	-
**************************************************************************/

static void ParseText_StripTrailingSpace(char *value, size_t *len)
{
	while (*len > 0 && ch_isspace(value[*len - 1])) {
		value[--*len] = '\0';
	}
}


/**************************************************************************
*** Function:	Parse_Text
***				Transforms any kind of linebreaks to '\n' (or ' ')
***				and all WS to space. Cuts off trailing WS.
*** Parameters: sgfc	 ... pointer to SGFInfo
***				v		 ... pointer to property value structure
***				prop_num ... prop value 1 or prop value 2
***				flags	 ... PVT_SIMPLE (SimpleText type)
***							 PVT_COMPOSE (compose type)
*** Returns:	length of converted string (0 for empty string)
**************************************************************************/

int Parse_Text(struct SGFInfo *sgfc, struct PropValue *v, int prop_num, uint16_t flags)
{
	char **value_ptr = &v->value;
	size_t *value_len = &v->value_len;
	if (prop_num == 2)
	{
		value_ptr = &v->value2;
		value_len = &v->value2_len;
	}

	ParseText_Unescape(*value_ptr, value_len);
	if(sgfc->options->encoding == OPTION_ENCODING_TEXT_ONLY)
		if(!ParseText_Decode(sgfc, value_ptr, value_len))
			return 0;
	ParseText_NormalizeWhitespace(sgfc, *value_ptr, value_len, v->row, v->col);
	ParseText_ApplyLinebreakStyle(sgfc, *value_ptr, value_len, flags);
	ParseText_StripTrailingSpace(*value_ptr, value_len);

	return (int)(*value_len);
}


/**************************************************************************
*** Function:	Parse_Number
***				Checks for illegal chars and for LONG INT range
*** Parameters: value ... pointer to value string
***				len		... length of string
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Number(char *value, size_t *len, ...)
{
	long i;
	parse_result_t ret = PARSE_OK;
	char *d;

	if(KillChars(value, len, C_NOTinSET, "+-0123456789"))
		ret = PARSE_CORRECTED_ERROR;

	if(*len)							/* empty? */
	{
		errno = 0;
		i = strtol(value, &d, 10);

		if(*d)							/* if *d: d >= value + 1 */
		{
			*d = 0;
			*len = strlen(value);
			if(*len)	ret = PARSE_CORRECTED_ERROR;
			else		ret = PARSE_ERROR;
		}

		if(errno == ERANGE)				/* out of range? */
		{
			sprintf(value, "%ld", i);	/* set to max range value */
			*len = strlen(value);
			ret = PARSE_CORRECTED_ERROR;
		}
	}
	else
		ret = PARSE_ERROR;

	return ret;
}


/**************************************************************************
*** Function:	Parse_Move
***				Kills illegal chars, checks position (board size)
***				transforms FF[3] PASS 'tt' into FF[4] PASS ''
*** Parameters: value ... pointer to value string
***				len		... length of string
***				flags ... PARSE_MOVE or PARSE_POS (treats 'tt' as error)
***				sgfc  ... pointer to SGFInfo
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Move(char *value, size_t *len, ...)
{
	parse_result_t ret = PARSE_OK;
	int c;
	unsigned int flags;
	bool emptyOrSpace = false;
	struct SGFInfo *sgfc;
	uint32_t *error_code;
	va_list arglist;

	va_start(arglist, len);
	flags = va_arg(arglist, unsigned int);
	sgfc = va_arg(arglist, struct SGFInfo *);
	error_code = va_arg(arglist, uint32_t *);
	va_end(arglist);

	if(sgfc->info->GM != 1)			/* game != GO ? */
	{
		ParseText_Unescape(value, len);
		if (KillChars(value, len, C_inSET, "\x00"))
			return PARSE_CORRECTED_ERROR;
		return PARSE_OK;
	}

	/* At first only delete space so that we can distinguish
	 * FF4 pass move from erroneous property values */
	if(KillChars(value, len, C_ISSPACE, NULL))
		ret = PARSE_CORRECTED_ERROR;
	if(!*len)
		emptyOrSpace = true;

	if(KillChars(value, len, C_NOT_ISALPHA, NULL))
		ret = PARSE_CORRECTED_ERROR;

	if(!*len)				/* empty value? */
	{
		if(flags & PARSE_MOVE && emptyOrSpace)
		{
			if(sgfc->info->FF >= 4)
				return ret;
			/* new pass '[]' in old FF[1-3], possible cause: missing FF */
			if(error_code)
				*error_code = E_FF4_PASS_IN_OLD_FF;
			return PARSE_CORRECTED_ERROR;
		}
		return PARSE_ERROR;
	}

	if(*len < 2)			/* value too short */
		return PARSE_ERROR;

	if(*len != 2)			/* value too long? */
	{
		*(value+2) = 0;
		*len = 2;
		ret = PARSE_CORRECTED_ERROR;
	}

	if((flags & PARSE_MOVE) && !strcmp(value, "tt"))
	{
		if(sgfc->info->bwidth <= 19 && sgfc->info->bheight <= 19)
		{
			*value = 0;					/* new pass */
			*len = 0;
			return ret;
		}
	}

	c = DecodePosChar(*value);
	if(!c)								/* check range */
		return PARSE_ERROR;
	if(c > sgfc->info->bwidth)
		return PARSE_ERROR;

	c = DecodePosChar(*(value+1));
	if(!c)
		return PARSE_ERROR;
	if(c > sgfc->info->bheight)
		return PARSE_ERROR;

	return ret;
}


/**************************************************************************
*** Function:	Parse_Float
***				Checks for correct float format / tries to correct
*** Parameters: value ... pointer to value string
***				len		... length of string
***				flags ... TYPE_GINFO => disallow '-' and '+' characters
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Float(char *value, size_t *len, ...)
{
	parse_result_t ret = PARSE_OK;
	int where = 0;
	unsigned int flags;
	/* where (bits): 0-minus / 1-int / 2-fraction / 3-'.' / 4-plus */
	char *s, *d;
	char *allowed;
	va_list arglist;

	va_start(arglist, len);
	flags = va_arg(arglist, unsigned int);
	va_end(arglist);
	allowed = (flags & TYPE_GINFO) ? "0123456789.," : "0123456789+-.,";

	if(KillChars(value, len, C_NOTinSET, allowed))
		ret = PARSE_CORRECTED_ERROR;

	s = d = value;
	while(*s)
	{
		switch(*s)
		{
			case '+':	if(where)	ret = PARSE_CORRECTED_ERROR;	/* '+' gets swallowed */
						else	{
									where = 16;
									ret = PARSE_CORRECTED;
								}
						break;
			case '-':	if(where)	ret = PARSE_CORRECTED_ERROR;
						else	{
									*d++ = *s;
									where = 1;
								}
						break;
			case ',':	ret = PARSE_CORRECTED_ERROR;
						*s = '.';
						ATTRIBUTE_FALLTHROUGH;
			case '.':	if(where & 8)	ret = PARSE_CORRECTED_ERROR;
						else	{
										*d++ = *s;
										where |= 8;
								}
						break;
			default:	if(where & 8)	where |= 4;
						else			where |= 2;
						*d++ = *s;
						break;
		}
		s++;
	}

	*d = 0;
	*len = strlen(value);

	if(!*len || !(where & 6))	/* empty || no digits? */
		ret = PARSE_ERROR;
	else
	{
		if((where & 8) && !(where & 2))		/* missing '0' in front of '.' */
		{
			size_t i = *len;
			ret = PARSE_CORRECTED_ERROR;
			d = value + i;
			s = d - 1;

			*(d+1) = 0;
			for(; i; i--)
				*d-- = *s--;
			(*len)++;

			if(where & 1)	*(value+1) = '0';	/* minus? */
			else			*value = '0';
		}

		if((where & 8) && (where & 4))	/* check for unnecessary '0' */
		{
			int mod = 0;	/* if correction occurred */
			d = value + *len - 1;

			while(*d == '0')
			{
				*d-- = 0;
				(*len)--;
				mod = 1;
			}

			if(*d == '.')
			{
				*d = 0;
				(*len)--;
				mod = 1;
			}

			if(ret == PARSE_OK && mod == 1)
				ret = PARSE_CORRECTED;
		}

		if((where & 8) && !(where & 4))		/* '.' without digits following */
		{
			ret = PARSE_CORRECTED_ERROR;
			(*len)--;
			*(value + *len) = 0;
		}
	}

	return ret;
}


/**************************************************************************
*** Function:	Parse_Float_Offset
***				Wrapper for easier handling of calling Parse_Float()
***				with an offset into the string
*** Parameters: value	... pointer to value string
***				len		... length of string
***				offset  ... offset into string for parser start
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Float_Offset(char *value, size_t *len, size_t offset)
{
	size_t len_offset = *len - offset;
	parse_result_t result = Parse_Float(&value[offset], &len_offset, TYPE_GINFO);
	*len = len_offset + offset;
	return result;
}


/**************************************************************************
*** Function:	Parse_Color
***				Checks & corrects color value
*** Parameters: value	... pointer to value string
***				len		... length of string
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Color(char *value, size_t *len, ...)
{
	parse_result_t ret = PARSE_OK;

	if(KillChars(value, len, C_NOTinSET, "BbWw"))
		ret = PARSE_CORRECTED_ERROR;

	switch(*value)
	{
		case 'B':
		case 'W':	break;
		case 'b':	*value = 'B';	/* uppercase required */
					ret = PARSE_CORRECTED_ERROR;
					break;
		case 'w':	*value = 'W';
					ret = PARSE_CORRECTED_ERROR;
					break;
		default:	return PARSE_ERROR;		/* unknown char -> error */
	}

	if(*len != 1)			/* string too long? */
	{
		*(value+1) = 0;
		(*len) = 1;
		ret = PARSE_CORRECTED_ERROR;
	}

	return ret;
}


/**************************************************************************
*** Function:	Parse_Triple
***				Checks & corrects triple value
*** Parameters: value	... pointer to value string
***				len		... length of string
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Triple(char *value, size_t *len, ...)
{
	parse_result_t ret = PARSE_OK;

	if(KillChars(value, len, C_NOTinSET, "12"))
		ret = PARSE_CORRECTED_ERROR;

	if(!*len)
	{
		*value = '1';
		*(value+1) = 0;
		*len = 1;
		ret = PARSE_CORRECTED_ERROR;
	}

	if(*value != '1' && *value != '2')
		return PARSE_ERROR;

	if(*len != 1)		/* string too long? */
	{
		*(value+1) = 0;
		*len = 1;
		ret = PARSE_CORRECTED_ERROR;
	}

	return ret;
}


/**************************************************************************
*** Function:	Parse_Charset
***				Checks & corrects charset values
*** Parameters: value	... pointer to value string
***				len		... length of string
*** Returns:	parse_result_t enum
**************************************************************************/

parse_result_t Parse_Charset(char *value, size_t *len, ...)
{
	parse_result_t ret = PARSE_OK;

	if(KillChars(value, len, C_NOTinSET, "-_:.0123456789"
										 "abcdefghijklmnopqrstuvwxyz"
										 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
		ret = PARSE_CORRECTED_ERROR;

	if(!*len)
		return PARSE_ERROR;
	return ret;
}


/**************************************************************************
*** Function:	Check_Value // Check_Single_Value (helper)
***				Checks value type & prints error messages
*** Parameters: sgfc	... pointer to SGFInfo structure
***				p		... pointer to property containing the value
***				v		... pointer to property value
***				flags	... flags to be passed on to parse function
***				Parse_Value ... function used for parsing
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

static bool Check_Single_Value(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v,
							   char *value, size_t *value_len, uint16_t flags,
							   parse_result_t (*Parse_Value)(char *, size_t *, ...))
{
	uint32_t error_code = E_BAD_VALUE_CORRECTED;
	char *before = SafeDupString(value, "prop value before checking");
	parse_result_t result = (*Parse_Value)(value, value_len, flags, sgfc, &error_code);

	switch(result)
	{
		case PARSE_CORRECTED_ERROR:
			PrintError(error_code, sgfc, v->row, v->col, before, p->idstr, value);
			break;
		case PARSE_ERROR:
			PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
			free(before);
			return false;
		case PARSE_OK:
		case PARSE_CORRECTED:
			break;
	}
	free(before);
	return true;
}

bool Check_Value(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v,
				 uint16_t flags, parse_result_t (*Parse_Value)(char *, size_t *, ...))
{
	if (!Check_Single_Value(sgfc, p, v, v->value, &v->value_len, flags, Parse_Value))
		return false;

	/* If there's a compose value, then parse the second value like the first one */
	if (flags & (PVT_COMPOSE|PVT_WEAKCOMPOSE) && v->value2)
		return Check_Single_Value(sgfc, p, v, v->value2, &v->value2_len, flags, Parse_Value);

	return true;
}


/**************************************************************************
*** Function:	Check_Text
***				Checks type value & prints error messages
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_Text(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	int value_len, value2_len = 0;

	value_len = Parse_Text(sgfc, v, 1, p->flags);
	if (p->flags & (PVT_COMPOSE|PVT_WEAKCOMPOSE) && v->value2)
	{
		value2_len = Parse_Text(sgfc, v, 2, p->flags);
	}

	if(!value_len && !value2_len && (p->flags & PVT_DEL_EMPTY))
	{
		PrintError(W_EMPTY_VALUE_DELETED, sgfc, v->row, v->col, p->idstr, "found");
		return false;
	}
	return true;
}


/**************************************************************************
*** Function:	Check_Pos
***				Checks position type & expand compressed point lists
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_Pos(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	if(!Check_Value(sgfc, p, v, PARSE_POS, Parse_Move))
		return false;

	if(v->value2)	/* compressed point list */
	{
		if(sgfc->info->FF < 4)
			PrintError(E_VERSION_CONFLICT, sgfc, v->row, v->col, sgfc->info->FF);

		switch(Parse_Move(v->value2, &v->value2_len, PARSE_POS, sgfc, v))
		{
			case PARSE_CORRECTED_ERROR:
						PrintError(E_BAD_VALUE_CORRECTED, sgfc, v->row, v->col, v->value, p->idstr, v->value2);
						break;
			case PARSE_ERROR:
						PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, v->value, p->idstr);
						return false;
			case PARSE_OK:
			case PARSE_CORRECTED:
						break;
		}

		if(sgfc->info->GM == 1)
			return !ExpandPointList(sgfc, p, v, true);
	}

	return true;
}


/**************************************************************************
*** Function:	Check_Stone
***				Checks stone type & expand compressed point lists if possible
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_Stone(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	if(sgfc->info->GM == 1)
		return Check_Pos(sgfc, p, v);

	if(v->value2)
	{
		/* stone type was erroneously split by load.c into composed value -> merge again */
		size_t new_len = safe_add3(v->value_len, v->value2_len, 1);
		char *stone_value = SafeMalloc(safe_add(new_len, 1), "property value buffer");
		memcpy(stone_value, v->value, v->value_len);
		memcpy(stone_value + v->value_len + 1, v->value2, v->value2_len);
		stone_value[v->value_len] = ':';					/* restore colon */
		stone_value[new_len] = 0;							/* 0-terminate */
		free(v->value);
		free(v->value2);
		v->value = stone_value;
		v->value_len = new_len;
		v->value2 = NULL;
		v->value2_len = 0;
	}

	/* Parse_Move does the right thing */
	return Check_Value(sgfc, p, v, PARSE_POS, Parse_Move);
}


/**************************************************************************
*** Function:	Check_Label
***				Checks label type value & prints error messages
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_Label(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	int error = 0;
	bool result = false;

	size_t before_size = safe_add3(v->value_len, v->value2_len, 2);
	char *before = SafeMalloc(before_size, "AR_LN value");
	sprintf(before, "%s:%s", v->value, v->value2);

	switch(Parse_Move(v->value, &v->value_len, PARSE_POS, sgfc, v))
	{
		case PARSE_ERROR:
					PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
					goto done;
		case PARSE_CORRECTED_ERROR:
					error = 1;
					ATTRIBUTE_FALLTHROUGH;
		case PARSE_OK:
		case PARSE_CORRECTED:
					switch(Parse_Text(sgfc, v, 2, p->flags))
					{
						case 0:	PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
								goto done;
						case 1:	if(v->value2_len > 4 && sgfc->info->FF < 4)
								{
									error = 1;
									*(v->value2+4) = 0;
									v->value2_len = 4;
								}
								break;
					}
					if(error)
						PrintError(E_BAD_COMPOSE_CORRECTED, sgfc, v->row, v->col, before,
								   p->idstr, v->value, v->value2);
					break;
	}
	result = true;

done:
	free(before);
	return result;
}


/**************************************************************************
*** Function:	Check_AR_LN
***				Checks arrow/line type values & prints error messages
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_AR_LN(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	int error = 0;
	bool result = false;

	size_t before_size = safe_add3(v->value_len, v->value2_len, 2);
	char *before = SafeMalloc(before_size, "AR_LN value");
	sprintf(before, "%s:%s", v->value, v->value2);

	switch(Parse_Move(v->value, &v->value_len, PARSE_POS, sgfc, v))
	{
		case PARSE_ERROR:
					PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
					goto done;
		case PARSE_CORRECTED_ERROR:
					error = 1;
					ATTRIBUTE_FALLTHROUGH;
		case PARSE_OK:
		case PARSE_CORRECTED:
					switch(Parse_Move(v->value2, &v->value2_len, PARSE_POS, sgfc, v))
					{
						case PARSE_ERROR:
								PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
								goto done;
						case PARSE_CORRECTED_ERROR:
								error = 1;
								ATTRIBUTE_FALLTHROUGH;
						case PARSE_OK:
						case PARSE_CORRECTED:
								if(!strcmp(v->value, v->value2))
								{
									PrintError(E_BAD_VALUE_DELETED, sgfc, v->row, v->col, before, p->idstr);
									goto done;
								}
								break;
					}
					if(error)
						PrintError(E_BAD_COMPOSE_CORRECTED, sgfc,
								   v->row, v->col, before, p->idstr, v->value, v->value2);
					break;
	}
	result = true;

done:
	free(before);
	return result;
}


/**************************************************************************
*** Function:	Check_Figure
***				Check FG property values
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property containing the value
***				v	 ... pointer to property value
*** Returns:	true for success / false if value has to be deleted
**************************************************************************/

bool Check_Figure(struct SGFInfo *sgfc, struct Property *p, struct PropValue *v)
{
	if(!v->value2)	/* no compose type */
	{
		if(v->value_len)
		{
			if(!Parse_Text(sgfc, v, 1, PVT_SIMPLE|PVT_COMPOSE))
				PrintError(E_BAD_VALUE_CORRECTED, sgfc, v->row, v->col, v->value, "FG", "");
			else
			{
				v->value2 = v->value;
				v->value = SafeMalloc(4, "new FG number value");
				strcpy(v->value, "0");
				PrintError(E_BAD_COMPOSE_CORRECTED, sgfc, v->row, v->col, v->value, "FG", v->value, v->value2);
			}
		}
	}
	else
	{
		Parse_Text(sgfc, v, 2, PVT_SIMPLE|PVT_COMPOSE);
		switch(Parse_Number(v->value, &v->value_len))
		{
			case PARSE_ERROR:
					strcpy(v->value, "0");
					ATTRIBUTE_FALLTHROUGH;
			case PARSE_CORRECTED_ERROR:
					PrintError(E_BAD_COMPOSE_CORRECTED, sgfc, v->row, v->col, v->value,
							   "FG", v->value, v->value2);
					break;
			case PARSE_CORRECTED:
			case PARSE_OK:
					break;
		}
	}

	return true;
}


/**************************************************************************
*** Function:	Check_PropValues
***				Checks values for syntax errors (calls Check_XXX functions)
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to property
*** Returns:	-
**************************************************************************/

static void Check_PropValues(struct SGFInfo *sgfc, struct Property *p)
{
	struct PropValue *v;

	v = p->value;
	while(v)
	{
		if(!v->value_len && !(p->flags & PVT_CHECK_EMPTY))
		{
			if(sgf_token[p->id].flags & PVT_DEL_EMPTY)
			{
				PrintError(W_EMPTY_VALUE_DELETED, sgfc, v->row, v->col, p->idstr, "found");
				v = DelPropValue(p, v);
			}
			else if(!(p->flags & PVT_EMPTY))
			{
				PrintError(E_EMPTY_VALUE_DELETED, sgfc, v->row, v->col, p->idstr, "not allowed");
				v = DelPropValue(p, v);
			}
			else
				v = v->next;
		}
		else
			if(sgf_token[p->id].CheckValue)
			{
				if((*sgf_token[p->id].CheckValue)(sgfc, p, v))
					v = v->next;
				else
					v = DelPropValue(p, v);
			}
			else
				v = v->next;
	}
}


/**************************************************************************
*** Function:	CheckID_Lowercase
***				Checks if the property ID contains lowercase letters (FF[4])
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... property to check
*** Returns:	-
**************************************************************************/

static void CheckID_Lowercase(struct SGFInfo *sgfc, struct Property *p)
{
	char *id = p->idstr;

	while(ch_isalpha(*id))
	{
		if(ch_islower(*id))
		{
			PrintError(E_LC_IN_PROPID, sgfc, p->row, p->col, p->idstr);
			break;		/* print error only once */
		}
		id++;
	}
}


/**************************************************************************
*** Function:	Check_Properties
***				Performs various checks on properties ID's
***				and calls Check_PropValues
*** Parameters: sgfc ... pointer to SGFInfo structure
***				p	 ... pointer to node containing the properties
***				st	 ... pointer to board status
*** Returns:	-
**************************************************************************/

void Check_Properties(struct SGFInfo *sgfc, struct Node *n, struct BoardStatus *st)
{
	struct Property *p, *hlp;
	int capped_ff = sgfc->info->FF <= 4 ? sgfc->info->FF : 4;

	p = n->prop;
	while(p)						/* property loop */
	{
		if((!(sgf_token[p->id].ff & (1 << (capped_ff - 1)))) &&
			 (p->id != TKN_KI))
		{
			if(sgf_token[p->id].data & ST_OBSOLETE)
				PrintError(WS_PROPERTY_NOT_IN_FF, sgfc, p->row, p->col,
						   p->idstr, sgfc->info->FF, "converted");
			else
				PrintError(WS_PROPERTY_NOT_IN_FF, sgfc, p->row, p->col,
						   p->idstr, sgfc->info->FF, "parsing done anyway");
		}

		if(!sgfc->options->keep_obsolete_props && !(sgf_token[p->id].ff & FF4) &&
		   !(sgf_token[p->id].data & ST_OBSOLETE))
		{
			PrintError(W_PROPERTY_DELETED, sgfc, p->row, p->col, "obsolete ", p->idstr);
			p = DelProperty(n, p);
			continue;
		}

		if(sgfc->info->FF >= 4)
			CheckID_Lowercase(sgfc, p);

		Check_PropValues(sgfc, p);

		if(!p->value)				/* all values of property deleted? */
			p = DelProperty(n, p);	/* -> del property */
		else
		{
			hlp = p->next;

			if(sgf_token[p->id].Execute_Prop)
			{
				if(!(*sgf_token[p->id].Execute_Prop)(sgfc, n, p, st) || !p->value)
					DelProperty(n, p);
			}

			p = hlp;
		}
	}
}
