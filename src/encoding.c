/**************************************************************************
*** Project: SGF Syntax Checker & Converter
***	File:	 encoding.c
***
*** Copyright (C) 1996-2018 by Arno Hollosi
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

#define STREAM_IN_CHUNK_SIZE 16
#define STREAM_OUT_BUFFER_SIZE 65  /* must be at least 4*IN_CHUNK_SIZE */

struct StreamReader
{
	iconv_t cd;
	char *in_buffer;
	char *in_pos;
	char *in_end;
	char out_buffer[STREAM_OUT_BUFFER_SIZE];
	char *out_pos;
	char *out_cursor;
};


char *DetectEncoding(struct SGFInfo *sgfc)
{
	const char *c = sgfc->buffer;
	int state = 1, brace_state = 1;

	if(c+3 >= sgfc->b_end)
		/* no encoding found (not even enough place for BOM) --> assume default */
		return NULL;

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
	 * has ASCII characters at ASCII codepoints, i.e. we can search for (CA[].
	 * Note: this done _before_ FindStart() so this might pick up
	 * text before the real SGF start. */

	while(c < sgfc->b_end && state)
	{
		switch(*c)
		{
			case '(':	state = 2;
						brace_state = 2;
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
				else if (isspace(*c))
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

	if(state)
		return NULL;	/* no encoding found -> assume default */

	/* c points to first char after "CA[" */
	while(c < sgfc->b_end && isspace(*c))
		c++;
	const char *c_end = c;
	while(c_end < sgfc->b_end && *c_end != ']')
		c_end++;

	if(c_end == sgfc->b_end)
		return NULL;	/* no encoding found -> assume default */

	do {
		c_end--;
	} while(isspace(*c_end));
	c_end++;

	/* c: first non-space in CA[] value, c_end: last non-space inside CA[] */
	return SaveDupString(c, c_end - c, "encoding");
}


char *DecodeBuffer(struct SGFInfo *sgfc, iconv_t cd,
				   char *in_buffer, size_t in_size,
				   U_LONG row, U_LONG col,
				   char **buffer_end)
{
	char *out_buffer, *out_pos;
	size_t in_left, out_size, result, out_left, err_left = -1;
	bool resize_out = false, add_replacement = false;
	bool illegal_sequence_error = false;

	out_size = in_left = in_size;
	/* +1 for \0 termination of buffer */
	SaveMalloc(char *, out_buffer, out_size + 1, "buffer for encoding conversion")
	out_pos = out_buffer;
	out_left = out_size;
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
					PrintError(WS_ENCODING_ERRORS, sgfc, in_buffer - sgfc->buffer); // FIXME: row/col??
				}
				in_buffer++;
				in_left--;
				if(err_left != in_left+1)	 /* squash follow up errors */
				{
					add_replacement = true;
					resize_out = out_left >= 3;
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
				char *new_buffer;
				/* +1 for \0 termination of buffer */
				SaveMalloc(char *, new_buffer, new_size+1, "temporary buffer for encoding conversion")
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
			PrintError(FE_ENCODING_ERROR, sgfc);
			return NULL;
		}
	}
	*out_pos = 0;  /* \0-terminate for convenience */
	if(buffer_end)
		*buffer_end = out_pos;
	return out_buffer;
}


bool DecodeWholeSGFBuffer(struct SGFInfo *sgfc)
{
	char *encoding;
	char *encoded_buffer, *encbuffer_end;

	if(sgfc->options->forced_encoding)
		encoding = SaveDupString(sgfc->options->forced_encoding, 0, "encoding name");
	else
	{
		encoding = DetectEncoding(sgfc);
		if(!encoding)
			encoding = SaveDupString(sgfc->options->default_encoding, 0, "encoding name");
	}
	// FIXME
	//encoded_buffer = ConvertEncoding(sgfc, encoding, &encbuffer_end);
	free(encoding);

	if(!encoded_buffer)
		return false;

	free(sgfc->buffer);				/* switch buffers */
	sgfc->buffer = encoded_buffer;
	sgfc->b_end = encbuffer_end;
	return true;
}

iconv_t DetectTreeEncoding(struct SGFInfo *sgfc, struct Node *n)
{
	const char *encoding;
	struct Property *ca = NULL;

	if(sgfc->options->forced_encoding)
		encoding = sgfc->options->forced_encoding;
	else
	{
		ca = FindProperty(n, TKN_CA);
		if(ca)
			encoding = ca->value->value;
		else
			encoding = sgfc->options->default_encoding;
	}

	iconv_t cd = iconv_open("UTF-8", encoding);
	while(cd == (iconv_t)-1)	/* options have been verified, so it should be CA property */
	{
		if(ca)
		{
			cd = iconv_open("UTF-8", sgfc->options->default_encoding);
			PrintError(0, sgfc);	// FIXME: error for unknown charset property, falling back to default CA
			ca = NULL;
			continue;
		}
		PrintError(0, sgfc); 	// FIXME: really unexpected: can no longer create cd (but verified!)
		return NULL;
	}
	return cd;
}


bool DecodeTextPropertyValues(struct SGFInfo *sgfc, struct Node *n, iconv_t cd)
{
	struct Property *p;
	struct PropValue *v;
	bool is_root = !n->parent;

	if(is_root)
		cd = DetectTreeEncoding(sgfc, n);
	if(!cd)
		return false;	/* fatal error */

	while(n)
	{
		if(n->sibling && !DecodeTextPropertyValues(sgfc, n->sibling, cd))
			return false;

		for(p = n->prop; p; p = p->next)
		{
			if(!(p->flags & PVT_TEXT))
				continue;
			if(!n->parent && p->id == TKN_CA) /* ignore CA[] property itself: will be replaced anyway */
				continue;

			for(v = p->value; v; v = v->next)
			{
				char *decoded = DecodeBuffer(sgfc, cd, p->value->value, v->value_len, v->row, v->col, NULL);
				if(decoded)
				{
					free(p->value->value);
					p->value->value = decoded;
				}
				// FIXME: what to do in error case? delete Prop? or at least propvalue? additional PrintError?
				if(p->value->value2)
				{
					decoded = DecodeBuffer(sgfc, cd, p->value->value2, v->value2_len, v->row, v->col, NULL);
					if(decoded)
					{
						free(p->value->value2);
						p->value->value2 = decoded;
					}
				}
			}
		}

		n = n->child;
	}

	if(is_root)
		iconv_close(cd);

	return true;
}


struct StreamReader *InitStreamReader(const char *encoding, char *in_buffer, char *in_end)
{
	struct StreamReader *sr;
	iconv_t cd;

	cd = iconv_open("UTF-8", encoding);
	if(cd == (iconv_t)-1)
		return NULL;
	SaveMalloc(struct StreamReader *, sr, sizeof(struct StreamReader), "stream reader");
	sr->cd = cd;
	sr->in_buffer = in_buffer;
	sr->in_pos = in_buffer;
	sr->in_end = in_end;
	sr->out_pos = sr->out_buffer;
	sr->out_cursor = sr->out_buffer;
	return sr;
}


int StreamDecode(struct StreamReader *sr)
{
	size_t in_left, result, out_left, err_left = -1;

	sr->out_pos = sr->out_buffer;
	out_left = STREAM_OUT_BUFFER_SIZE;
	in_left = sr->in_end - sr->in_pos;
	if(!in_left)
		return 3;
	if(in_left > STREAM_IN_CHUNK_SIZE)
		in_left = STREAM_IN_CHUNK_SIZE;

	while(in_left)
	{
		result = iconv(sr->cd, &sr->in_pos, &in_left, &sr->out_pos, &out_left);
		if(result == (size_t)-1)
		{
			if(errno == EILSEQ)	/* illegal bytes found */
			{
				sr->in_pos++;
				in_left--;
				if(err_left != in_left+1)	 /* squash follow up errors */
				{
					*sr->out_pos++ = (char)0xEF; /* UTF-8 encoded replacement character */
					*sr->out_pos++ = (char)0xBF; /* U+FFFD */
					*sr->out_pos++ = (char)0xBD;
					out_left -= 3;
				}
				err_left = in_left;
				continue;
			}
			if(errno == EINVAL) /* incomplete sequence */
			{
				return 0;
			}
			return 2;
		}
	}
	return 0;
}

int NextDecodedChar(struct StreamReader *sr)
{
	/* still bytes in the out_buffer? */
	if(sr->out_cursor >= sr->out_pos) /* FIXME?? off-by-1 */
	{
		int result = StreamDecode(sr);
		if(result == 3)
			return EOF;
		sr->out_cursor = sr->out_buffer;
	}
	/* read/decode next bytes from in_buffer */
	return *sr->out_cursor++;
}
