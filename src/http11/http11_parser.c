
#line 1 "src/http11/http11_parser.rl"
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

#include "http11_parser.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dbg.h>

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)

/** Machine **/


#line 169 "src/http11/http11_parser.rl"


/** Data **/

#line 58 "src/http11/http11_parser.c"
static const int http_parser_start = 1;
static const int http_parser_first_final = 95;
static const int http_parser_error = 0;

static const int http_parser_en_main = 1;


#line 173 "src/http11/http11_parser.rl"

int http_parser_init(http_parser *parser) {
  int cs = 0;
  
#line 71 "src/http11/http11_parser.c"
	{
	cs = http_parser_start;
	}

#line 177 "src/http11/http11_parser.rl"
  parser->cs = cs;
  parser->body_start = 0;
  parser->content_len = 0;
  parser->mark = 0;
  parser->nread = 0;
  parser->field_len = 0;
  parser->field_start = 0;
  parser->socket_started = 0;
  parser->json_sent = 0;

  return(1);
}


/** exec **/
size_t http_parser_execute(http_parser *parser, const char *buffer, size_t len, size_t off)  
{
  if(len == 0) return 0;

  const char *p, *pe;
  int cs = parser->cs;

  assert(off <= len && "offset past end of buffer");

  p = buffer+off;
  pe = buffer+len;

  assert(pe - p == len - off && "pointers aren't same distance");

  
#line 107 "src/http11/http11_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 36: goto tr0;
		case 60: goto st60;
		case 64: goto tr3;
		case 95: goto tr0;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto tr0;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto tr0;
	} else
		goto tr0;
	goto st0;
st0:
cs = 0;
	goto _out;
tr0:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 140 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st41;
		case 95: goto st41;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st41;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st41;
	} else
		goto st41;
	goto st0;
tr4:
#line 69 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_method != NULL) 
      parser->request_method(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 166 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 42: goto tr6;
		case 47: goto tr7;
		case 104: goto tr8;
	}
	goto st0;
tr6:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 181 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr9;
		case 35: goto tr10;
	}
	goto st0;
tr9:
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
tr35:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
#line 79 "src/http11/http11_parser.rl"
	{
    if(parser->fragment != NULL)
      parser->fragment(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
tr38:
#line 79 "src/http11/http11_parser.rl"
	{
    if(parser->fragment != NULL)
      parser->fragment(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
tr42:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
tr53:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
tr57:
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 252 "src/http11/http11_parser.c"
	if ( (*p) == 72 )
		goto tr11;
	goto st0;
tr11:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 264 "src/http11/http11_parser.c"
	if ( (*p) == 84 )
		goto st7;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 84 )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 80 )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 47 )
		goto st10;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 49 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 46 )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( 48 <= (*p) && (*p) <= 49 )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( (*p) ) {
		case 10: goto tr19;
		case 13: goto tr20;
	}
	goto st0;
tr19:
#line 90 "src/http11/http11_parser.rl"
	{	
    if(parser->http_version != NULL)
      parser->http_version(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st14;
tr27:
#line 61 "src/http11/http11_parser.rl"
	{ MARK(mark, p); }
#line 63 "src/http11/http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
  }
	goto st14;
tr31:
#line 63 "src/http11/http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
  }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 348 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 10: goto tr21;
		case 13: goto st15;
		case 33: goto tr23;
		case 124: goto tr23;
		case 126: goto tr23;
	}
	if ( (*p) < 45 ) {
		if ( (*p) > 39 ) {
			if ( 42 <= (*p) && (*p) <= 43 )
				goto tr23;
		} else if ( (*p) >= 35 )
			goto tr23;
	} else if ( (*p) > 46 ) {
		if ( (*p) < 65 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr23;
		} else if ( (*p) > 90 ) {
			if ( 94 <= (*p) && (*p) <= 122 )
				goto tr23;
		} else
			goto tr23;
	} else
		goto tr23;
	goto st0;
tr21:
#line 100 "src/http11/http11_parser.rl"
	{ 
    parser->body_start = p - buffer + 1; 
    if(parser->header_done != NULL)
      parser->header_done(parser->data, p + 1, pe - p - 1);
    {p++; cs = 95; goto _out;}
  }
	goto st95;
tr106:
#line 107 "src/http11/http11_parser.rl"
	{
      parser->socket_started = 1;
  }
#line 100 "src/http11/http11_parser.rl"
	{ 
    parser->body_start = p - buffer + 1; 
    if(parser->header_done != NULL)
      parser->header_done(parser->data, p + 1, pe - p - 1);
    {p++; cs = 95; goto _out;}
  }
	goto st95;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
#line 400 "src/http11/http11_parser.c"
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 10 )
		goto tr21;
	goto st0;
tr23:
#line 56 "src/http11/http11_parser.rl"
	{ MARK(field_start, p); }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 417 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 33: goto st16;
		case 58: goto tr25;
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
tr25:
#line 57 "src/http11/http11_parser.rl"
	{ 
    parser->field_len = LEN(field_start, p);
  }
	goto st17;
tr29:
#line 61 "src/http11/http11_parser.rl"
	{ MARK(mark, p); }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 456 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 10: goto tr27;
		case 13: goto tr28;
		case 32: goto tr29;
	}
	goto tr26;
tr26:
#line 61 "src/http11/http11_parser.rl"
	{ MARK(mark, p); }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 471 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 10: goto tr31;
		case 13: goto tr32;
	}
	goto st18;
tr20:
#line 90 "src/http11/http11_parser.rl"
	{	
    if(parser->http_version != NULL)
      parser->http_version(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st19;
tr28:
#line 61 "src/http11/http11_parser.rl"
	{ MARK(mark, p); }
#line 63 "src/http11/http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
  }
	goto st19;
tr32:
#line 63 "src/http11/http11_parser.rl"
	{
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
    }
  }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 506 "src/http11/http11_parser.c"
	if ( (*p) == 10 )
		goto st14;
	goto st0;
tr10:
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st20;
tr43:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st20;
tr54:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st20;
tr58:
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 74 "src/http11/http11_parser.rl"
	{ 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 559 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr35;
		case 37: goto tr36;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto tr34;
tr34:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 581 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr38;
		case 37: goto st22;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto st21;
tr36:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 603 "src/http11/http11_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st23;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st23;
	} else
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st21;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st21;
	} else
		goto st21;
	goto st0;
tr7:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 634 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr42;
		case 34: goto st0;
		case 35: goto tr43;
		case 37: goto st25;
		case 59: goto tr45;
		case 60: goto st0;
		case 62: goto st0;
		case 63: goto tr46;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st24;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st26;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st26;
	} else
		goto st26;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st24;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st24;
	} else
		goto st24;
	goto st0;
tr45:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 686 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr9;
		case 34: goto st0;
		case 35: goto tr10;
		case 37: goto st28;
		case 60: goto st0;
		case 62: goto st0;
		case 63: goto st30;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st27;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st29;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st29;
	} else
		goto st29;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st27;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st27;
	} else
		goto st27;
	goto st0;
tr46:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 737 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr53;
		case 34: goto st0;
		case 35: goto tr54;
		case 37: goto tr55;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto tr52;
tr52:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 758 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr57;
		case 34: goto st0;
		case 35: goto tr58;
		case 37: goto st32;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st31;
tr55:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
	goto st32;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
#line 779 "src/http11/http11_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st33;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st33;
	} else
		goto st33;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st31;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st31;
	} else
		goto st31;
	goto st0;
tr8:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 810 "src/http11/http11_parser.c"
	if ( (*p) == 116 )
		goto st35;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 116 )
		goto st36;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) == 112 )
		goto st37;
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	if ( (*p) == 58 )
		goto st38;
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 32: goto tr9;
		case 34: goto st0;
		case 35: goto tr10;
		case 37: goto st39;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st38;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st40;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st40;
	} else
		goto st40;
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st38;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st38;
	} else
		goto st38;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st42;
		case 95: goto st42;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st42;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st42;
	} else
		goto st42;
	goto st0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st43;
		case 95: goto st43;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st43;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st43;
	} else
		goto st43;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st44;
		case 95: goto st44;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st44;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st44;
	} else
		goto st44;
	goto st0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st45;
		case 95: goto st45;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st45;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st45;
	} else
		goto st45;
	goto st0;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st46;
		case 95: goto st46;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st46;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st46;
	} else
		goto st46;
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st47;
		case 95: goto st47;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st47;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st47;
	} else
		goto st47;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st48;
		case 95: goto st48;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st48;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st48;
	} else
		goto st48;
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st49;
		case 95: goto st49;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st49;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st49;
	} else
		goto st49;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st50;
		case 95: goto st50;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st50;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st50;
	} else
		goto st50;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st51;
		case 95: goto st51;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st51;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st51;
	} else
		goto st51;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st52;
		case 95: goto st52;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st52;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st52;
	} else
		goto st52;
	goto st0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st53;
		case 95: goto st53;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st53;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st53;
	} else
		goto st53;
	goto st0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st54;
		case 95: goto st54;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st54;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st54;
	} else
		goto st54;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st55;
		case 95: goto st55;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st55;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st55;
	} else
		goto st55;
	goto st0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st56;
		case 95: goto st56;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st56;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st56;
	} else
		goto st56;
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st57;
		case 95: goto st57;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st57;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st57;
	} else
		goto st57;
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st58;
		case 95: goto st58;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st58;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st58;
	} else
		goto st58;
	goto st0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 32: goto tr4;
		case 36: goto st59;
		case 95: goto st59;
	}
	if ( (*p) < 48 ) {
		if ( 45 <= (*p) && (*p) <= 46 )
			goto st59;
	} else if ( (*p) > 57 ) {
		if ( 65 <= (*p) && (*p) <= 90 )
			goto st59;
	} else
		goto st59;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 32 )
		goto tr4;
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	if ( (*p) == 112 )
		goto st61;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 111 )
		goto st62;
	goto st0;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	if ( (*p) == 108 )
		goto st63;
	goto st0;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	if ( (*p) == 105 )
		goto st64;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	if ( (*p) == 99 )
		goto st65;
	goto st0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	if ( (*p) == 121 )
		goto st66;
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	if ( (*p) == 45 )
		goto st67;
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	if ( (*p) == 102 )
		goto st68;
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 105 )
		goto st69;
	goto st0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	if ( (*p) == 108 )
		goto st70;
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	if ( (*p) == 101 )
		goto st71;
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	if ( (*p) == 45 )
		goto st72;
	goto st0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	if ( (*p) == 114 )
		goto st73;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	if ( (*p) == 101 )
		goto st74;
	goto st0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	if ( (*p) == 113 )
		goto st75;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 117 )
		goto st76;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	if ( (*p) == 101 )
		goto st77;
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	if ( (*p) == 115 )
		goto st78;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	if ( (*p) == 116 )
		goto st79;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	if ( (*p) == 47 )
		goto st80;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	if ( (*p) == 62 )
		goto st81;
	goto st0;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	if ( (*p) == 0 )
		goto tr106;
	goto st0;
tr3:
#line 53 "src/http11/http11_parser.rl"
	{MARK(mark, p); }
	goto st82;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
#line 1370 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr108;
		case 37: goto st86;
		case 47: goto st0;
		case 59: goto tr110;
		case 60: goto st0;
		case 62: goto st0;
		case 63: goto tr111;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto st83;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	switch( (*p) ) {
		case 32: goto tr108;
		case 37: goto st86;
		case 59: goto tr110;
		case 60: goto st0;
		case 62: goto st0;
		case 63: goto tr111;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto st83;
tr108:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st84;
tr120:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st84;
tr123:
#line 85 "src/http11/http11_parser.rl"
	{ 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, p));
  }
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st84;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
#line 1443 "src/http11/http11_parser.c"
	if ( (*p) == 123 )
		goto st85;
	goto st0;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( (*p) == 0 )
		goto tr113;
	goto st85;
tr113:
#line 111 "src/http11/http11_parser.rl"
	{
      parser->json_sent = 1;
  }
#line 100 "src/http11/http11_parser.rl"
	{ 
    parser->body_start = p - buffer + 1; 
    if(parser->header_done != NULL)
      parser->header_done(parser->data, p + 1, pe - p - 1);
    {p++; cs = 96; goto _out;}
  }
	goto st96;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
#line 1471 "src/http11/http11_parser.c"
	if ( (*p) == 0 )
		goto tr113;
	goto st85;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st87;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st87;
	} else
		goto st87;
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st83;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st83;
	} else
		goto st83;
	goto st0;
tr110:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st88;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
#line 1512 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr108;
		case 37: goto st89;
		case 60: goto st0;
		case 62: goto st0;
		case 63: goto st91;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto st88;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st90;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st90;
	} else
		goto st90;
	goto st0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st88;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st88;
	} else
		goto st88;
	goto st0;
tr111:
#line 95 "src/http11/http11_parser.rl"
	{
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,p));
  }
	goto st91;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
#line 1564 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr120;
		case 37: goto tr121;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto tr119;
tr119:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
	goto st92;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
#line 1586 "src/http11/http11_parser.c"
	switch( (*p) ) {
		case 32: goto tr123;
		case 37: goto st93;
		case 60: goto st0;
		case 62: goto st0;
		case 127: goto st0;
	}
	if ( (*p) > 31 ) {
		if ( 34 <= (*p) && (*p) <= 35 )
			goto st0;
	} else if ( (*p) >= 0 )
		goto st0;
	goto st92;
tr121:
#line 84 "src/http11/http11_parser.rl"
	{MARK(query_start, p); }
	goto st93;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
#line 1608 "src/http11/http11_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st94;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st94;
	} else
		goto st94;
	goto st0;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st92;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto st92;
	} else
		goto st92;
	goto st0;
	}
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
	_test_eof95: cs = 95; goto _test_eof; 
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
	_test_eof96: cs = 96; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 
	_test_eof91: cs = 91; goto _test_eof; 
	_test_eof92: cs = 92; goto _test_eof; 
	_test_eof93: cs = 93; goto _test_eof; 
	_test_eof94: cs = 94; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 207 "src/http11/http11_parser.rl"

  assert(p <= pe && "Buffer overflow after parsing.");

  if (!http_parser_has_error(parser)) {
      parser->cs = cs;
  }

  parser->nread += p - (buffer + off);

  assert(parser->nread <= len && "nread longer than length");
  assert(parser->body_start <= len && "body starts after buffer end");
  assert(parser->mark < len && "mark is after buffer end");
  assert(parser->field_len <= len && "field has length longer than whole buffer");
  assert(parser->field_start < len && "field starts after buffer end");

  return(parser->nread);
}

int http_parser_finish(http_parser *parser)
{
  if (http_parser_has_error(parser) ) {
    return -1;
  } else if (http_parser_is_finished(parser) ) {
    return 1;
  } else {
    return 0;
  }
}

int http_parser_has_error(http_parser *parser) {
  return parser->cs == http_parser_error;
}

int http_parser_is_finished(http_parser *parser) {
  return parser->cs >= http_parser_first_final;
}
