/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 encoding.c
***
*** Copyright (C) 1996-2020 by Arno Hollosi
*** (see 'main.c' for more copyright information)
***
**************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <math.h>

#include "all.h"
#include "protos.h"


/**************************************************************************
*** Function:	OpenIconV
***				Safely opens an iconv descriptor for the given encoding
***				Falls back to default_encoding if specified encoding fails,
***				forced_encoding overrides any provided encoding.
*** Parameters: sgfc		  ... pointer to SGFInfo structure
***				encoding	  ... name of desired encoding or NULL for default
***				encoding_name ... output parameter: holds name of selected encoding
*** Returns:	pointer to iconv descriptor
**************************************************************************/

iconv_t OpenIconV(struct SGFInfo *sgfc, const char *encoding, const char **encoding_name)
{
	iconv_t cd;

	if(sgfc->options->forced_encoding)
	{
		cd = iconv_open("UTF-8", sgfc->options->forced_encoding);
		if(encoding_name)
			*encoding_name = sgfc->options->forced_encoding;
	}
	else
	{
		if(encoding)
		{
			cd = iconv_open("UTF-8", encoding);
			if(encoding_name)
				*encoding_name = encoding;
			if(cd != (iconv_t)-1)
				return cd;
			PrintError(WS_ENCODING_FALLBACK, sgfc, encoding, sgfc->options->default_encoding);
		}
		cd = iconv_open("UTF-8", sgfc->options->default_encoding);
		if(encoding_name)
			*encoding_name = sgfc->options->default_encoding;
	}

	if(cd != (iconv_t)-1)
		return cd;

	PrintError(FE_ENCODING_ERROR, sgfc, 0);
	return NULL;
}


/**************************************************************************
*** Function:	DetectEncoding
***				Searches for CA[] property in buffer; starts from sgfc->current
***				Contains mini-parser which might pick up different CA[] than load.c
*** Parameters: c	  ... start of buffer
***				b_end ... end of buffer
*** Returns:	pointer to encoding name (needs to be freed) or NULL
**************************************************************************/

char *DetectEncoding(const char *c, const char *b_end)
{
	int state = 1, brace_state = 1, brace_count = 0;

	if(c+3 >= b_end)
		/* no encoding found (not even enough place for BOM) --> assume default */
		return NULL;
	if(c+1000 < b_end)
		b_end = c+1000;	/* limit search to first 1000 bytes */

	/* check for Unicode BOM */
	if(*c == (char)0xFE && *(c+1) == (char)0xFF)
		return SaveDupString("UTF-16BE", 0, "encoding");
	if(*c == (char)0xFF && *(c+1) == (char)0xFE)
	{
		if(!*(c+2) && !*(c+3))
			return SaveDupString("UTF-32LE", 0, "encoding");
		return SaveDupString("UTF-16LE", 0, "encoding");
	}
	if(!*c && !*(c+1) && *(c+2) == (char)0xFE && *(c+3) == (char)0xFF)
		return SaveDupString("UTF-32BE", 0, "encoding");
	if(*c == (char)0xEF && *(c+1) == (char)0xBB && *(c+2) == (char)0xBF)
		return SaveDupString("UTF-8", 0, "encoding");

	/* assume that while not necessarily ASCII-safe, that the encoding
	 * has ASCII characters at ASCII codepoints, i.e. we can search for "(CA[]".
	 * Note: this done _before_ FindStart() so this might pick up
	 * text before the real SGF start. */
	while(c < b_end && state)
	{
		switch(*c)
		{
			case '(':	state = 2;
						brace_state = 2;
						brace_count++;
						break;
			case 'C':	if(state == 2) state = 3;
						else		   state = brace_state;
						break;
			case 'A':	if(state == 3) state = 4;
						else		   state = brace_state;
						break;
			case '[':	if(state == 4) state = 0;
						else		   state = brace_state;
						break;
			default:
				if(isupper(*c))
					state = brace_state;
				else if(isspace(*c))
				{
					if(state != 4)
						state = brace_state;
				}
				else if(!islower(*c))
					state = brace_state;
				break;
		}
		c++;
	}

	/* if we found more than three open braces, chances are that the CA[] property
	 * is not part of current game tree, but some game tree afterwards. So we
	 * ignore the CA[] property in this case.
	 */
	if(state || brace_count >= 3)
		return NULL;	/* no encoding found -> assume default */

	/* c points to first char after "CA[" */
	const char *c_end = c;
	while(c_end < b_end && *c_end != ']')
		c_end++;
	size_t len = c_end - c;
	char *ca_value = SaveDupString(c, len, "encoding");
	if(!Parse_Charset(ca_value, &len) || !len)
	{
		free(ca_value);
		return NULL;
	}
	return ca_value;
}


/**************************************************************************
*** Function:	DecodeBuffer
***				Decodes specified buffer using provided iconv descriptor.
***				Scales output buffer incrementally, but might
***				might allocate more memory than strictly necessary.
*** Parameters: sgfc		... pointer to SGFInfo structure
***				cd			... iconv conversion descriptor
***				buffer		... start of input buffer
***				size		... size of input buffer
***				err_offset	... byte offset for error reporting purposes
***				buffer_end	... output variable: end of decoded buffer
*** Returns:	pointer to decoded buffer or NULL
**************************************************************************/

char *DecodeBuffer(struct SGFInfo *sgfc, iconv_t cd,
				   char *buffer, size_t size, U_LONG err_offset,
				   const char **buffer_end)
{
	char *out_buffer, *out_pos, *in_buffer;
	size_t in_left, out_size, result, out_left, err_left = -1;
	bool resize_out = false, add_replacement = false;
	bool illegal_sequence_error = false;

	in_buffer = buffer;
	out_size = in_left = size;
	/* +1 for \0 termination of buffer */
	out_buffer = SaveMalloc(out_size + 1, "buffer for encoding conversion");
	out_pos = out_buffer;
	out_left = out_size;

	iconv(cd, NULL, 0, NULL, 0);    /* reset internal iconv state */

	while(in_left)
	{
		result = iconv(cd, &in_buffer, &in_left, &out_pos, &out_left);
		if(result == (size_t)-1)
		{
			if(errno == EINVAL || errno == EILSEQ)	/* illegal bytes found */
			{
				if(!illegal_sequence_error)
				{
					illegal_sequence_error = true;
					PrintError(WS_ENCODING_ERRORS, sgfc, in_buffer - buffer + err_offset);
				}
				in_buffer++;
				in_left--;
				if(err_left != in_left+1)	 /* squash follow up errors */
				{
					add_replacement = true;
					resize_out = out_left <= 3;
				}
				err_left = in_left;
				if(!add_replacement)
					continue;
			}

			if(errno == E2BIG || resize_out)	/* destination buffer is full */
			{
				/* bytes needed are estimated based on encoding progress so far
				 * +1 because of edge case of out_size==in_left */
				float needed = (float)in_left * (float)out_size / (float)(out_size - in_left + 1);
				size_t increase = (size_t)(lrintf(needed*1.05)) + 12; /* +5% + 3x 4 byte wide chars */
				size_t new_size = out_size + increase;
				/* +1 for \0 termination of buffer */
				char *new_buffer = SaveMalloc(new_size+1, "temporary buffer for encoding conversion");
				memcpy(new_buffer, out_buffer, out_size);
				out_pos = new_buffer + (out_pos - out_buffer);
				out_left += increase;
				free(out_buffer);
				out_buffer = new_buffer;
				out_size = new_size;
				if(!resize_out)
					continue;
				resize_out = false;
			}

			if(add_replacement)	/* add replacement character for illegal chars */
			{
				*out_pos++ = (char)0xEF; /* UTF-8 encoded replacement character */
				*out_pos++ = (char)0xBF; /* U+FFFD */
				*out_pos++ = (char)0xBD;
				out_left -= 3;
				add_replacement = false;
				continue;
			}

			free(out_buffer);
			iconv_close(cd);
			PrintError(FE_ENCODING_ERROR, sgfc, in_buffer - buffer + err_offset);
			return NULL;
		}
	}
	*out_pos = 0;  /* \0-terminate for convenience */
	if(buffer_end)
		*buffer_end = out_pos;
	return out_buffer;
}


/**************************************************************************
*** Function:	DecodeSGFBuffer
***				Decodes complete SGF buffer
*** Parameters: sgfc		  ... pointer to SGFInfo structure
***				encbuffer_end ... output variable: end of decoded buffer
***				encoding_name ... output variable: name of encoding used
*** Returns:	pointer to decoded buffer or NULL in case of fatal error
**************************************************************************/

char *DecodeSGFBuffer(struct SGFInfo *sgfc, const char **encbuffer_end, char **encoding_name)
{
	char *encoding = DetectEncoding(sgfc->buffer, sgfc->b_end);		/* might be NULL! */
	const char *selected_encoding;
	iconv_t cd = OpenIconV(sgfc, encoding, &selected_encoding);
	if(encoding != selected_encoding)
	{
		free(encoding);
		*encoding_name = SaveDupString(selected_encoding, 0, "encoding name");
	}
	else
		*encoding_name = encoding;
	if(!cd)
		return NULL;

	char *encoded_buffer = DecodeBuffer(sgfc, cd, sgfc->buffer, sgfc->b_end - sgfc->buffer,
										0, encbuffer_end);
	iconv_close(cd);
	return encoded_buffer;
}
