
#line 1 "src/httpclient_parser.rl"
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

#line 124 "src/httpclient_parser.rl"


/** Data **/

#line 58 "src/httpclient_parser.c"
static const char _httpclient_parser_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 10, 2, 2, 3, 
	2, 3, 4, 2, 9, 10, 2, 10, 
	9, 3, 2, 3, 4
};

static const short _httpclient_parser_key_offsets[] = {
	0, 0, 10, 11, 19, 20, 28, 29, 
	44, 62, 77, 94, 109, 127, 142, 159, 
	174, 192, 207, 224, 225, 226, 227, 228, 
	230, 233, 235, 238, 240, 243, 243, 244, 
	245, 261, 277, 279, 280
};

static const char _httpclient_parser_trans_keys[] = {
	13, 48, 59, 72, 49, 57, 65, 70, 
	97, 102, 10, 13, 59, 48, 57, 65, 
	70, 97, 102, 10, 13, 59, 48, 57, 
	65, 70, 97, 102, 10, 33, 124, 126, 
	35, 39, 42, 43, 45, 46, 48, 57, 
	65, 90, 94, 122, 13, 33, 59, 61, 
	124, 126, 35, 39, 42, 43, 45, 46, 
	48, 57, 65, 90, 94, 122, 33, 124, 
	126, 35, 39, 42, 43, 45, 46, 48, 
	57, 65, 90, 94, 122, 13, 33, 59, 
	124, 126, 35, 39, 42, 43, 45, 46, 
	48, 57, 65, 90, 94, 122, 33, 124, 
	126, 35, 39, 42, 43, 45, 46, 48, 
	57, 65, 90, 94, 122, 13, 33, 59, 
	61, 124, 126, 35, 39, 42, 43, 45, 
	46, 48, 57, 65, 90, 94, 122, 33, 
	124, 126, 35, 39, 42, 43, 45, 46, 
	48, 57, 65, 90, 94, 122, 13, 33, 
	59, 124, 126, 35, 39, 42, 43, 45, 
	46, 48, 57, 65, 90, 94, 122, 33, 
	124, 126, 35, 39, 42, 43, 45, 46, 
	48, 57, 65, 90, 94, 122, 13, 33, 
	59, 61, 124, 126, 35, 39, 42, 43, 
	45, 46, 48, 57, 65, 90, 94, 122, 
	33, 124, 126, 35, 39, 42, 43, 45, 
	46, 48, 57, 65, 90, 94, 122, 13, 
	33, 59, 124, 126, 35, 39, 42, 43, 
	45, 46, 48, 57, 65, 90, 94, 122, 
	84, 84, 80, 47, 48, 57, 46, 48, 
	57, 48, 57, 32, 48, 57, 48, 57, 
	32, 48, 57, 13, 10, 13, 33, 124, 
	126, 35, 39, 42, 43, 45, 46, 48, 
	57, 65, 90, 94, 122, 33, 58, 124, 
	126, 35, 39, 42, 43, 45, 46, 48, 
	57, 65, 90, 94, 122, 13, 32, 13, 
	0
};

static const char _httpclient_parser_single_lengths[] = {
	0, 4, 1, 2, 1, 2, 1, 3, 
	6, 3, 5, 3, 6, 3, 5, 3, 
	6, 3, 5, 1, 1, 1, 1, 0, 
	1, 0, 1, 0, 1, 0, 1, 1, 
	4, 4, 2, 1, 0
};

static const char _httpclient_parser_range_lengths[] = {
	0, 3, 0, 3, 0, 3, 0, 6, 
	6, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	6, 6, 0, 0, 0
};

static const unsigned char _httpclient_parser_index_offsets[] = {
	0, 0, 8, 10, 16, 18, 24, 26, 
	36, 49, 59, 71, 81, 94, 104, 116, 
	126, 139, 149, 161, 163, 165, 167, 169, 
	171, 174, 176, 179, 181, 184, 185, 187, 
	189, 200, 211, 214, 216
};

static const char _httpclient_parser_indicies[] = {
	0, 2, 4, 5, 3, 3, 3, 1, 
	6, 1, 7, 9, 8, 8, 8, 1, 
	10, 1, 11, 12, 8, 8, 8, 1, 
	13, 1, 14, 14, 14, 14, 14, 14, 
	14, 14, 14, 1, 15, 16, 17, 18, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	1, 19, 19, 19, 19, 19, 19, 19, 
	19, 19, 1, 20, 21, 22, 21, 21, 
	21, 21, 21, 21, 21, 21, 1, 23, 
	23, 23, 23, 23, 23, 23, 23, 23, 
	1, 24, 25, 26, 27, 25, 25, 25, 
	25, 25, 25, 25, 25, 1, 28, 28, 
	28, 28, 28, 28, 28, 28, 28, 1, 
	29, 30, 31, 30, 30, 30, 30, 30, 
	30, 30, 30, 1, 32, 32, 32, 32, 
	32, 32, 32, 32, 32, 1, 33, 34, 
	35, 36, 34, 34, 34, 34, 34, 34, 
	34, 34, 1, 37, 37, 37, 37, 37, 
	37, 37, 37, 37, 1, 38, 39, 40, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	1, 41, 1, 42, 1, 43, 1, 44, 
	1, 45, 1, 46, 45, 1, 47, 1, 
	48, 47, 1, 49, 1, 50, 51, 1, 
	52, 54, 53, 55, 1, 56, 57, 57, 
	57, 57, 57, 57, 57, 57, 57, 1, 
	58, 59, 58, 58, 58, 58, 58, 58, 
	58, 58, 1, 61, 62, 60, 64, 63, 
	1, 0
};

static const char _httpclient_parser_trans_targs[] = {
	2, 0, 3, 5, 15, 19, 36, 4, 
	5, 11, 36, 6, 7, 36, 8, 6, 
	8, 7, 9, 10, 6, 10, 7, 12, 
	4, 12, 11, 13, 14, 4, 14, 11, 
	16, 2, 16, 15, 17, 18, 2, 18, 
	15, 20, 21, 22, 23, 24, 25, 26, 
	27, 28, 29, 28, 30, 30, 31, 32, 
	6, 33, 33, 34, 35, 31, 34, 35, 
	31
};

static const char _httpclient_parser_trans_actions[] = {
	0, 0, 1, 1, 0, 1, 27, 17, 
	0, 17, 30, 17, 17, 19, 3, 33, 
	0, 33, 21, 7, 9, 0, 9, 3, 
	33, 0, 33, 21, 7, 9, 0, 9, 
	3, 33, 0, 33, 21, 7, 9, 0, 
	9, 0, 0, 0, 0, 0, 0, 0, 
	15, 1, 13, 0, 1, 0, 11, 0, 
	0, 3, 0, 5, 7, 24, 7, 0, 
	9
};

static const int httpclient_parser_start = 1;
static const int httpclient_parser_first_final = 36;
static const int httpclient_parser_error = 0;

static const int httpclient_parser_en_main = 1;


#line 128 "src/httpclient_parser.rl"

int httpclient_parser_init(httpclient_parser *parser)  {
  int cs = 0;

  
#line 206 "src/httpclient_parser.c"
	{
	cs = httpclient_parser_start;
	}

#line 133 "src/httpclient_parser.rl"

  parser->cs = cs;
  parser->body_start = 0;
  parser->content_len = 0;
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


  
#line 241 "src/httpclient_parser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _httpclient_parser_trans_keys + _httpclient_parser_key_offsets[cs];
	_trans = _httpclient_parser_index_offsets[cs];

	_klen = _httpclient_parser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _httpclient_parser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _httpclient_parser_indicies[_trans];
	cs = _httpclient_parser_trans_targs[_trans];

	if ( _httpclient_parser_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _httpclient_parser_actions + _httpclient_parser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 52 "src/httpclient_parser.rl"
	{MARK(mark, p); }
	break;
	case 1:
#line 54 "src/httpclient_parser.rl"
	{ MARK(field_start, p); }
	break;
	case 2:
#line 56 "src/httpclient_parser.rl"
	{ 
    parser->field_len = LEN(field_start, p);
  }
	break;
	case 3:
#line 60 "src/httpclient_parser.rl"
	{ MARK(mark, p); }
	break;
	case 4:
#line 62 "src/httpclient_parser.rl"
	{ 
    parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, p));
  }
	break;
	case 5:
#line 66 "src/httpclient_parser.rl"
	{ 
    parser->reason_phrase(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	break;
	case 6:
#line 70 "src/httpclient_parser.rl"
	{ 
    parser->status_code(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	break;
	case 7:
#line 74 "src/httpclient_parser.rl"
	{	
    parser->http_version(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	break;
	case 8:
#line 78 "src/httpclient_parser.rl"
	{
    parser->chunk_size(parser->data, PTR_TO(mark), LEN(mark, p));
  }
	break;
	case 9:
#line 82 "src/httpclient_parser.rl"
	{
    parser->last_chunk(parser->data, NULL, 0);
  }
	break;
	case 10:
#line 86 "src/httpclient_parser.rl"
	{ 
    parser->body_start = p - buffer + 1; 
    if(parser->header_done != NULL)
      parser->header_done(parser->data, p + 1, pe - p - 1);
    {p++; goto _out; }
  }
	break;
#line 378 "src/httpclient_parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 162 "src/httpclient_parser.rl"

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
