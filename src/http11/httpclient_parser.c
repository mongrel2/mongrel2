
#line 1 "src/http11/httpclient_parser.rl"
/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "httpclient_parser.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)
#define L(M) fprintf(stderr, "" # M "\n");


/** machine **/

#line 135 "src/http11/httpclient_parser.rl"


/** Data **/

#line 58 "src/http11/httpclient_parser.c"
static const int httpclient_parser_start = 1;
static const int httpclient_parser_first_final = 36;
static const int httpclient_parser_error = 0;

static const int httpclient_parser_en_main = 1;


#line 139 "src/http11/httpclient_parser.rl"

int httpclient_parser_init(httpclient_parser *parser)  {
    int cs = 0;

    
#line 72 "src/http11/httpclient_parser.c"
	{
	cs = httpclient_parser_start;
	}

#line 144 "src/http11/httpclient_parser.rl"

    parser->cs = cs;
    parser->body_start = 0;
    parser->content_len = 0;
    parser->chunked = 0;
    parser->chunks_done = 0;
    parser->mark = 0;
    parser->nread = 0;
    parser->field_len = 0;
    parser->field_start = 0;    

    return(1);
}


/** exec **/
size_t httpclient_parser_execute(httpclient_parser *parser, const char *buffer, size_t len, size_t off)  
{
    const char *p, *pe;
    int cs = parser->cs;

    assert(off <= len && "offset past end of buffer");

    p = buffer+off;
    pe = buffer+len;

    assert(*pe == '\0' && "pointer does not end on NUL");
    assert(pe - p == len - off && "pointers aren't same distance");


    
#line 109 "src/http11/httpclient_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 10: goto tr0;
		case 13: goto st2;
		case 48: goto tr3;
		case 59: goto st15;
		case 72: goto tr6;
	}
	if ( (*p) < 65 ) {
		if ( 49 <= (*p) && (*p) <= 57 )
			goto tr4;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr4;
	} else
		goto tr4;
	goto st0;
st0:
cs = 0;
	goto _out;
tr0:
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr7:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
	goto st36;
tr11:
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
	goto st36;
tr12:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr15:
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr17:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr23:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr28:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
	goto st36;
tr34:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
	goto st36;
tr39:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
tr45:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
#line 90 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = 0;
        parser->chunks_done = 1;
        parser->last_chunk(parser->data, NULL, 0);
    }
#line 97 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 36; goto _out;}
    }
	goto st36;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
#line 351 "src/http11/httpclient_parser.c"
	goto st0;
tr40:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st2;
tr46:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 377 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto tr0;
	goto st0;
tr3:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 389 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr7;
		case 13: goto tr8;
		case 59: goto tr10;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st5;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st5;
	} else
		goto st5;
	goto st0;
tr8:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st4;
tr29:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st4;
tr35:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 438 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto tr11;
	goto st0;
tr4:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 450 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr12;
		case 13: goto tr13;
		case 59: goto tr14;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st5;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st5;
	} else
		goto st5;
	goto st0;
tr13:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st6;
tr18:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st6;
tr24:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 499 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto tr15;
	goto st0;
tr14:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st7;
tr20:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st7;
tr26:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 537 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr16;
		case 124: goto tr16;
		case 126: goto tr16;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr16;
		} else if ( (*p) >= 35 )
			goto tr16;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr16;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr16;
		} else
			goto tr16;
	} else
		goto tr16;
	goto st0;
tr16:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 569 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr17;
		case 13: goto tr18;
		case 33: goto st8;
		case 59: goto tr20;
		case 61: goto tr21;
		case 124: goto st8;
		case 126: goto st8;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st8;
		} else if ( (*p) >= 35 )
			goto st8;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st8;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st8;
		} else
			goto st8;
	} else
		goto st8;
	goto st0;
tr21:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 609 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr22;
		case 124: goto tr22;
		case 126: goto tr22;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr22;
		} else if ( (*p) >= 35 )
			goto tr22;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr22;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr22;
		} else
			goto tr22;
	} else
		goto tr22;
	goto st0;
tr22:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 641 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr23;
		case 13: goto tr24;
		case 33: goto st10;
		case 59: goto tr26;
		case 124: goto st10;
		case 126: goto st10;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st10;
		} else if ( (*p) >= 35 )
			goto st10;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st10;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st10;
		} else
			goto st10;
	} else
		goto st10;
	goto st0;
tr10:
#line 82 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;
        if(parser->chunk_size != NULL)
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st11;
tr31:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st11;
tr37:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 702 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr27;
		case 124: goto tr27;
		case 126: goto tr27;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr27;
		} else if ( (*p) >= 35 )
			goto tr27;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr27;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr27;
		} else
			goto tr27;
	} else
		goto tr27;
	goto st0;
tr27:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 734 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr28;
		case 13: goto tr29;
		case 33: goto st12;
		case 59: goto tr31;
		case 61: goto tr32;
		case 124: goto st12;
		case 126: goto st12;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st12;
		} else if ( (*p) >= 35 )
			goto st12;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st12;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st12;
		} else
			goto st12;
	} else
		goto st12;
	goto st0;
tr32:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 774 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr33;
		case 124: goto tr33;
		case 126: goto tr33;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr33;
		} else if ( (*p) >= 35 )
			goto tr33;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr33;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr33;
		} else
			goto tr33;
	} else
		goto tr33;
	goto st0;
tr33:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 806 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr34;
		case 13: goto tr35;
		case 33: goto st14;
		case 59: goto tr37;
		case 124: goto st14;
		case 126: goto st14;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st14;
		} else if ( (*p) >= 35 )
			goto st14;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st14;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st14;
		} else
			goto st14;
	} else
		goto st14;
	goto st0;
tr42:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st15;
tr48:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 857 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr38;
		case 124: goto tr38;
		case 126: goto tr38;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr38;
		} else if ( (*p) >= 35 )
			goto tr38;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr38;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr38;
		} else
			goto tr38;
	} else
		goto tr38;
	goto st0;
tr38:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 889 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr39;
		case 13: goto tr40;
		case 33: goto st16;
		case 59: goto tr42;
		case 61: goto tr43;
		case 124: goto st16;
		case 126: goto st16;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st16;
		} else if ( (*p) >= 35 )
			goto st16;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st16;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st16;
		} else
			goto st16;
	} else
		goto st16;
	goto st0;
tr43:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 929 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr44;
		case 124: goto tr44;
		case 126: goto tr44;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr44;
		} else if ( (*p) >= 35 )
			goto tr44;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr44;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr44;
		} else
			goto tr44;
	} else
		goto tr44;
	goto st0;
tr44:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 961 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 33: goto st18;
		case 59: goto tr48;
		case 124: goto st18;
		case 126: goto st18;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st18;
		} else if ( (*p) >= 35 )
			goto st18;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st18;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st18;
		} else
			goto st18;
	} else
		goto st18;
	goto st0;
tr6:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 996 "src/http11/httpclient_parser.c"
	if ( (*p) == 84 )
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( (*p) == 84 )
		goto st21;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 80 )
		goto st22;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( (*p) == 47 )
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st24;
	goto st0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 46 )
		goto st25;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st24;
	goto st0;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st26;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	if ( (*p) == 32 )
		goto tr56;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st26;
	goto st0;
tr56:
#line 77 "src/http11/httpclient_parser.rl"
	{	
        if(parser->http_version != NULL)
            parser->http_version(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 1064 "src/http11/httpclient_parser.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr57;
	goto st0;
tr57:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 1076 "src/http11/httpclient_parser.c"
	if ( (*p) == 32 )
		goto tr58;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st28;
	goto st0;
tr58:
#line 72 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->status_code != NULL)
            parser->status_code(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 1093 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto st0;
	goto tr60;
tr60:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 1105 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr62;
		case 13: goto tr63;
	}
	goto st30;
tr73:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st31;
tr62:
#line 67 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->reason_phrase != NULL)
            parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st31;
tr69:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 1138 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr15;
		case 13: goto st6;
		case 33: goto tr65;
		case 124: goto tr65;
		case 126: goto tr65;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr65;
		} else if ( (*p) >= 35 )
			goto tr65;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr65;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr65;
		} else
			goto tr65;
	} else
		goto tr65;
	goto st0;
tr65:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st32;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
#line 1172 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto st32;
		case 58: goto tr67;
		case 124: goto st32;
		case 126: goto st32;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st32;
		} else if ( (*p) >= 35 )
			goto st32;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st32;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st32;
		} else
			goto st32;
	} else
		goto st32;
	goto st0;
tr71:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st33;
tr67:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
	goto st33;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
#line 1211 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr69;
		case 13: goto tr70;
		case 32: goto tr71;
	}
	goto tr68;
tr68:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 1226 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr73;
		case 13: goto tr74;
	}
	goto st34;
tr74:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st35;
tr63:
#line 67 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->reason_phrase != NULL)
            parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st35;
tr70:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL)
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
	goto st35;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
#line 1259 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto st31;
	goto st0;
	}
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 175 "src/http11/httpclient_parser.rl"

    parser->cs = cs;
    parser->nread += p - (buffer + off);

    assert(p <= pe && "buffer overflow after parsing execute");
    assert(parser->nread <= len && "nread longer than length");
    assert(parser->body_start <= len && "body starts after buffer end");
    assert(parser->mark < len && "mark is after buffer end");
    assert(parser->field_len <= len && "field has length longer than whole buffer");
    assert(parser->field_start < len && "field starts after buffer end");

    if(parser->body_start) {
        /* final \r\n combo encountered so stop right here */
        parser->nread++;
    }

    return(parser->nread);
}

int httpclient_parser_finish(httpclient_parser *parser)
{
    int cs = parser->cs;

    parser->cs = cs;

    if (httpclient_parser_has_error(parser) ) {
        return -1;
    } else if (httpclient_parser_is_finished(parser) ) {
        return 1;
    } else {
        return 0;
    }
}

int httpclient_parser_has_error(httpclient_parser *parser) {
    return parser->cs == httpclient_parser_error;
}

int httpclient_parser_is_finished(httpclient_parser *parser) {
    return parser->cs == httpclient_parser_first_final;
}
