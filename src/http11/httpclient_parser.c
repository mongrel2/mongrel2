
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
#include "dbg.h"

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)


/** machine **/

#line 147 "src/http11/httpclient_parser.rl"


/** Data **/

#line 58 "src/http11/httpclient_parser.c"
static const int httpclient_parser_start = 1;
static const int httpclient_parser_first_final = 91;
static const int httpclient_parser_error = 0;

static const int httpclient_parser_en_main = 1;


#line 151 "src/http11/httpclient_parser.rl"

int httpclient_parser_init(httpclient_parser *parser)  {
    int cs = 0;

    
#line 72 "src/http11/httpclient_parser.c"
	{
	cs = httpclient_parser_start;
	}

#line 156 "src/http11/httpclient_parser.rl"

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
int httpclient_parser_execute(httpclient_parser *parser, const char *buffer, size_t len, size_t off)  
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
	if ( (*p) == 72 )
		goto tr2;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr0;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr0;
	} else
		goto tr0;
	goto st0;
st0:
cs = 0;
	goto _out;
tr0:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 138 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr3;
		case 13: goto tr4;
		case 59: goto tr6;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st2;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st2;
	} else
		goto st2;
	goto st0;
tr3:
#line 87 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;

        if(parser->chunks_done && parser->last_chunk) {
            parser->last_chunk(parser->data, PTR_TO(mark), LEN(mark, p));
        } else if(parser->chunk_size != NULL) {
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
        } // else skip it
    }
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 91; goto _out;}
    }
	goto st91;
tr7:
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 91; goto _out;}
    }
	goto st91;
tr9:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 91; goto _out;}
    }
	goto st91;
tr15:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 91; goto _out;}
    }
	goto st91;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
#line 223 "src/http11/httpclient_parser.c"
	goto st0;
tr4:
#line 87 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;

        if(parser->chunks_done && parser->last_chunk) {
            parser->last_chunk(parser->data, PTR_TO(mark), LEN(mark, p));
        } else if(parser->chunk_size != NULL) {
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
        } // else skip it
    }
	goto st3;
tr10:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st3;
tr16:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 265 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto tr7;
	goto st0;
tr6:
#line 87 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
        parser->content_len = strtol(PTR_TO(mark), NULL, 16);
        parser->chunks_done = parser->content_len <= 0;

        if(parser->chunks_done && parser->last_chunk) {
            parser->last_chunk(parser->data, PTR_TO(mark), LEN(mark, p));
        } else if(parser->chunk_size != NULL) {
            parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
        } // else skip it
    }
	goto st4;
tr12:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st4;
tr18:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 309 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr8;
		case 124: goto tr8;
		case 126: goto tr8;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr8;
		} else if ( (*p) >= 35 )
			goto tr8;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr8;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr8;
		} else
			goto tr8;
	} else
		goto tr8;
	goto st0;
tr8:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 341 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr9;
		case 13: goto tr10;
		case 33: goto st5;
		case 59: goto tr12;
		case 61: goto tr13;
		case 124: goto st5;
		case 126: goto st5;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st5;
		} else if ( (*p) >= 35 )
			goto st5;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st5;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st5;
		} else
			goto st5;
	} else
		goto st5;
	goto st0;
tr13:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 381 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto tr14;
		case 124: goto tr14;
		case 126: goto tr14;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr14;
		} else if ( (*p) >= 35 )
			goto tr14;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr14;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr14;
		} else
			goto tr14;
	} else
		goto tr14;
	goto st0;
tr14:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 413 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr15;
		case 13: goto tr16;
		case 33: goto st7;
		case 59: goto tr18;
		case 124: goto st7;
		case 126: goto st7;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st7;
		} else if ( (*p) >= 35 )
			goto st7;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st7;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st7;
		} else
			goto st7;
	} else
		goto st7;
	goto st0;
tr2:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 448 "src/http11/httpclient_parser.c"
	if ( (*p) == 84 )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 84 )
		goto st10;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 80 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 47 )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 46 )
		goto st14;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st13;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 32 )
		goto tr26;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st15;
	goto st0;
tr26:
#line 82 "src/http11/httpclient_parser.rl"
	{	
        if(parser->http_version != NULL)
            parser->http_version(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 516 "src/http11/httpclient_parser.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr27;
	goto st0;
tr27:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 528 "src/http11/httpclient_parser.c"
	if ( (*p) == 32 )
		goto tr28;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st17;
	goto st0;
tr28:
#line 77 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->status_code != NULL)
            parser->status_code(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 545 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto st0;
	goto tr30;
tr30:
#line 52 "src/http11/httpclient_parser.rl"
	{MARK(mark, p); }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 557 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr32;
		case 13: goto tr33;
	}
	goto st19;
tr45:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st20;
tr32:
#line 72 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->reason_phrase != NULL)
            parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st20;
tr42:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st20;
tr71:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        parser->content_len = strtol(PTR_TO(mark), NULL, 10);
    }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st20;
tr107:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
#line 99 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
    }
	goto st20;
tr109:
#line 99 "src/http11/httpclient_parser.rl"
	{
        parser->chunked = 1;
    }
	goto st20;
tr124:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        parser->content_len = strtol(PTR_TO(mark), NULL, 10);
    }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 634 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr7;
		case 13: goto st3;
		case 33: goto tr35;
		case 67: goto tr36;
		case 84: goto tr37;
		case 99: goto tr36;
		case 116: goto tr37;
		case 124: goto tr35;
		case 126: goto tr35;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr35;
		} else if ( (*p) >= 35 )
			goto tr35;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr35;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr35;
		} else
			goto tr35;
	} else
		goto tr35;
	goto st0;
tr35:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 672 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
tr41:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st22;
tr39:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 711 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr42;
		case 13: goto tr43;
		case 32: goto tr41;
	}
	if ( 9 <= (*p) && (*p) <= 12 )
		goto tr41;
	goto tr40;
tr40:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st23;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
#line 728 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
	}
	goto st23;
tr46:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st24;
tr33:
#line 72 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->reason_phrase != NULL)
            parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, p));
    }
	goto st24;
tr43:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st24;
tr72:
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        parser->content_len = strtol(PTR_TO(mark), NULL, 10);
    }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st24;
tr125:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
#line 62 "src/http11/httpclient_parser.rl"
	{ 
        parser->content_len = strtol(PTR_TO(mark), NULL, 10);
    }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 787 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto st20;
	goto st0;
tr36:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 799 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 79: goto st26;
		case 111: goto st26;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st27;
		case 110: goto st27;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 84: goto st28;
		case 116: goto st28;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 69: goto st29;
		case 101: goto st29;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st30;
		case 110: goto st30;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 84: goto st31;
		case 116: goto st31;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	switch( (*p) ) {
		case 33: goto st21;
		case 45: goto st32;
		case 46: goto st21;
		case 58: goto tr39;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 48 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else if ( (*p) >= 65 )
			goto st21;
	} else
		goto st21;
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 76: goto st33;
		case 108: goto st33;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 69: goto st34;
		case 101: goto st34;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st35;
		case 110: goto st35;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 71: goto st36;
		case 103: goto st36;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 84: goto st37;
		case 116: goto st37;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 72: goto st38;
		case 104: goto st38;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr61;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
tr62:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st39;
tr61:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 1225 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr63;
		case 13: goto tr64;
		case 32: goto tr62;
	}
	if ( (*p) > 12 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr65;
	} else if ( (*p) >= 9 )
		goto tr62;
	goto tr40;
tr63:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st40;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
#line 1251 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr67;
		case 13: goto st43;
		case 32: goto st41;
		case 33: goto tr35;
		case 67: goto tr36;
		case 84: goto tr37;
		case 99: goto tr36;
		case 116: goto tr37;
		case 124: goto tr35;
		case 126: goto tr35;
	}
	if ( (*p) < 45 ) {
		if ( (*p) < 35 ) {
			if ( 9 <= (*p) && (*p) <= 12 )
				goto st41;
		} else if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr35;
		} else
			goto tr35;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr69;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr35;
		} else
			goto tr35;
	} else
		goto tr35;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	if ( (*p) == 32 )
		goto st41;
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr70;
	} else if ( (*p) >= 9 )
		goto st41;
	goto st0;
tr70:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st42;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
#line 1305 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr71;
		case 13: goto tr72;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st42;
	goto st0;
tr67:
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 92; goto _out;}
    }
	goto st92;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
#line 1326 "src/http11/httpclient_parser.c"
	if ( (*p) == 32 )
		goto st41;
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr70;
	} else if ( (*p) >= 9 )
		goto st41;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 10: goto tr67;
		case 32: goto st41;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr70;
	} else if ( (*p) >= 9 )
		goto st41;
	goto st0;
tr69:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st44;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
#line 1359 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr71;
		case 13: goto tr72;
		case 33: goto st21;
		case 58: goto tr39;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st44;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
tr37:
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st45;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
#line 1394 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 82: goto st46;
		case 114: goto st46;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 65: goto st47;
		case 97: goto st47;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 66 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st48;
		case 110: goto st48;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 83: goto st49;
		case 115: goto st49;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 70: goto st50;
		case 102: goto st50;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 69: goto st51;
		case 101: goto st51;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 82: goto st52;
		case 114: goto st52;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	switch( (*p) ) {
		case 33: goto st21;
		case 45: goto st53;
		case 46: goto st21;
		case 58: goto tr39;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 48 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 57 ) {
		if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else if ( (*p) >= 65 )
			goto st21;
	} else
		goto st21;
	goto st0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 69: goto st54;
		case 101: goto st54;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st55;
		case 110: goto st55;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 67: goto st56;
		case 99: goto st56;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 79: goto st57;
		case 111: goto st57;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 68: goto st58;
		case 100: goto st58;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 73: goto st59;
		case 105: goto st59;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st60;
		case 110: goto st60;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 71: goto st61;
		case 103: goto st61;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr91;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
tr92:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st62;
tr91:
#line 56 "src/http11/httpclient_parser.rl"
	{ 
        parser->field_len = LEN(field_start, p);
    }
	goto st62;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
#line 1910 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr93;
		case 13: goto tr94;
		case 32: goto tr92;
		case 67: goto tr95;
		case 99: goto tr95;
	}
	if ( 9 <= (*p) && (*p) <= 12 )
		goto tr92;
	goto tr40;
tr93:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st63;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
#line 1935 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr97;
		case 13: goto st73;
		case 32: goto st64;
		case 33: goto tr35;
		case 67: goto tr99;
		case 84: goto tr37;
		case 99: goto tr99;
		case 116: goto tr37;
		case 124: goto tr35;
		case 126: goto tr35;
	}
	if ( (*p) < 45 ) {
		if ( (*p) < 35 ) {
			if ( 9 <= (*p) && (*p) <= 12 )
				goto st64;
		} else if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr35;
		} else
			goto tr35;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr35;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr35;
		} else
			goto tr35;
	} else
		goto tr35;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	switch( (*p) ) {
		case 32: goto st64;
		case 67: goto tr100;
		case 99: goto tr100;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st64;
	goto st0;
tr100:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st65;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
#line 1989 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 72: goto st66;
		case 104: goto st66;
	}
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	switch( (*p) ) {
		case 85: goto st67;
		case 117: goto st67;
	}
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	switch( (*p) ) {
		case 78: goto st68;
		case 110: goto st68;
	}
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	switch( (*p) ) {
		case 75: goto st69;
		case 107: goto st69;
	}
	goto st0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	switch( (*p) ) {
		case 69: goto st70;
		case 101: goto st70;
	}
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	switch( (*p) ) {
		case 68: goto st71;
		case 100: goto st71;
	}
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	switch( (*p) ) {
		case 10: goto tr107;
		case 13: goto tr108;
	}
	goto st0;
tr108:
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st72;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
#line 2061 "src/http11/httpclient_parser.c"
	if ( (*p) == 10 )
		goto tr109;
	goto st0;
tr97:
#line 103 "src/http11/httpclient_parser.rl"
	{ 
        parser->body_start = p - buffer + 1; 
        if(parser->header_done != NULL)
            parser->header_done(parser->data, p + 1, pe - p - 1);
        {p++; cs = 93; goto _out;}
    }
	goto st93;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
#line 2078 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 32: goto st64;
		case 67: goto tr100;
		case 99: goto tr100;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st64;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	switch( (*p) ) {
		case 10: goto tr97;
		case 32: goto st64;
		case 67: goto tr100;
		case 99: goto tr100;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st64;
	goto st0;
tr99:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 54 "src/http11/httpclient_parser.rl"
	{ MARK(field_start, p); }
	goto st74;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
#line 2110 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 72: goto st75;
		case 79: goto st26;
		case 104: goto st75;
		case 111: goto st26;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 85: goto st76;
		case 117: goto st76;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 78: goto st77;
		case 110: goto st77;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 75: goto st78;
		case 107: goto st78;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 69: goto st79;
		case 101: goto st79;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 33: goto st21;
		case 58: goto tr39;
		case 68: goto st80;
		case 100: goto st80;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	switch( (*p) ) {
		case 10: goto tr107;
		case 13: goto tr108;
		case 33: goto st21;
		case 58: goto tr39;
		case 124: goto st21;
		case 126: goto st21;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto st21;
		} else if ( (*p) >= 35 )
			goto st21;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto st21;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto st21;
		} else
			goto st21;
	} else
		goto st21;
	goto st0;
tr94:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st81;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
#line 2333 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto st63;
		case 32: goto st64;
		case 67: goto tr100;
		case 99: goto tr100;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st64;
	goto st0;
tr95:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st82;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
#line 2351 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 72: goto st83;
		case 104: goto st83;
	}
	goto st23;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 85: goto st84;
		case 117: goto st84;
	}
	goto st23;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 78: goto st85;
		case 110: goto st85;
	}
	goto st23;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 75: goto st86;
		case 107: goto st86;
	}
	goto st23;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 69: goto st87;
		case 101: goto st87;
	}
	goto st23;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
		case 68: goto st88;
		case 100: goto st88;
	}
	goto st23;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	switch( (*p) ) {
		case 10: goto tr107;
		case 13: goto tr108;
	}
	goto st23;
tr64:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
#line 66 "src/http11/httpclient_parser.rl"
	{ 
        if(parser->http_field != NULL) {
            parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
        }
    }
	goto st89;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
#line 2437 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto st40;
		case 32: goto st41;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr70;
	} else if ( (*p) >= 9 )
		goto st41;
	goto st0;
tr65:
#line 60 "src/http11/httpclient_parser.rl"
	{ MARK(mark, p); }
	goto st90;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
#line 2456 "src/http11/httpclient_parser.c"
	switch( (*p) ) {
		case 10: goto tr124;
		case 13: goto tr125;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st90;
	goto st23;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof91: cs = 91; goto _test_eof; 
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
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof92: cs = 92; goto _test_eof; 
	_test_eof43: cs = 43; goto _test_eof; 
	_test_eof44: cs = 44; goto _test_eof; 
	_test_eof45: cs = 45; goto _test_eof; 
	_test_eof46: cs = 46; goto _test_eof; 
	_test_eof47: cs = 47; goto _test_eof; 
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof51: cs = 51; goto _test_eof; 
	_test_eof52: cs = 52; goto _test_eof; 
	_test_eof53: cs = 53; goto _test_eof; 
	_test_eof54: cs = 54; goto _test_eof; 
	_test_eof55: cs = 55; goto _test_eof; 
	_test_eof56: cs = 56; goto _test_eof; 
	_test_eof57: cs = 57; goto _test_eof; 
	_test_eof58: cs = 58; goto _test_eof; 
	_test_eof59: cs = 59; goto _test_eof; 
	_test_eof60: cs = 60; goto _test_eof; 
	_test_eof61: cs = 61; goto _test_eof; 
	_test_eof62: cs = 62; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
	_test_eof67: cs = 67; goto _test_eof; 
	_test_eof68: cs = 68; goto _test_eof; 
	_test_eof69: cs = 69; goto _test_eof; 
	_test_eof70: cs = 70; goto _test_eof; 
	_test_eof71: cs = 71; goto _test_eof; 
	_test_eof72: cs = 72; goto _test_eof; 
	_test_eof93: cs = 93; goto _test_eof; 
	_test_eof73: cs = 73; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 
	_test_eof77: cs = 77; goto _test_eof; 
	_test_eof78: cs = 78; goto _test_eof; 
	_test_eof79: cs = 79; goto _test_eof; 
	_test_eof80: cs = 80; goto _test_eof; 
	_test_eof81: cs = 81; goto _test_eof; 
	_test_eof82: cs = 82; goto _test_eof; 
	_test_eof83: cs = 83; goto _test_eof; 
	_test_eof84: cs = 84; goto _test_eof; 
	_test_eof85: cs = 85; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 187 "src/http11/httpclient_parser.rl"

    parser->cs = cs;
    parser->nread += p - (buffer + off);

    assert(p <= pe && "buffer overflow after parsing execute");
    assert(parser->nread <= len && "nread longer than length");
    assert(parser->body_start <= len && "body starts after buffer end");
    check(parser->mark < len, "mark is after buffer end");
    check(parser->field_len <= len, "field has length longer than whole buffer");
    check(parser->field_start < len, "field starts after buffer end");

    if(parser->body_start) {
        /* final \r\n combo encountered so stop right here */
        parser->nread++;
    }

    return(parser->nread);

error:
    return -1;
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
