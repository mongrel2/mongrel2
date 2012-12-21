
#line 1 "src/handler_parser.rl"
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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "handler_parser.h"
#include "bstring.h"
#include "dbg.h"
#include "mem/halloc.h"


#line 90 "src/handler_parser.rl"


/** Data **/

#line 53 "src/handler_parser.c"
static const int HandlerParser_start = 1;
static const int HandlerParser_first_final = 11;
static const int HandlerParser_error = 0;

static const int HandlerParser_en_main = 1;


#line 94 "src/handler_parser.rl"


/** exec **/
int HandlerParser_execute(HandlerParser *parser, const char *buffer, size_t len)
{
    const char *p = buffer;
    const char *pe = buffer+len;
    int cs = 0;
    const char *mark = p;
    const char *targets_start = NULL;
    int target_expected_len = 0;
    parser->target_count = 0;
    parser->uuid = NULL;

    
#line 77 "src/handler_parser.c"
	{
	cs = HandlerParser_start;
	}

#line 109 "src/handler_parser.rl"
    
#line 84 "src/handler_parser.c"
	{
	switch ( cs )
	{
case 1:
	if ( (*p) == 45 )
		goto tr0;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr0;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr0;
	} else
		goto tr0;
	goto st0;
st0:
cs = 0;
	goto _out;
tr0:
#line 48 "src/handler_parser.rl"
	{ mark = p; }
	goto st2;
st2:
	p += 1;
case 2:
#line 110 "src/handler_parser.c"
	switch( (*p) ) {
		case 32: goto tr2;
		case 45: goto st2;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st2;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st2;
	} else
		goto st2;
	goto st0;
tr2:
#line 56 "src/handler_parser.rl"
	{
        parser->uuid = blk2bstr(mark, p-mark);
    }
	goto st3;
st3:
	p += 1;
case 3:
#line 133 "src/handler_parser.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr4;
	goto st0;
tr4:
#line 48 "src/handler_parser.rl"
	{ mark = p; }
	goto st4;
st4:
	p += 1;
case 4:
#line 144 "src/handler_parser.c"
	if ( (*p) == 58 )
		goto tr6;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st4;
	goto st0;
tr6:
#line 50 "src/handler_parser.rl"
	{
        char *endptr = NULL;
        target_expected_len = strtoul(mark, &endptr, 10);
        check(endptr == p, "Invalid length given, didn't parse correctly.");
    }
	goto st5;
st5:
	p += 1;
case 5:
#line 161 "src/handler_parser.c"
	switch( (*p) ) {
		case 44: goto tr7;
		case 88: goto tr9;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr8;
	goto st0;
tr7:
#line 69 "src/handler_parser.rl"
	{
        targets_start = p;
    }
#line 73 "src/handler_parser.rl"
	{
        check(p-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(p-targets_start), (int)target_expected_len);
    }
	goto st6;
tr12:
#line 60 "src/handler_parser.rl"
	{
        check(parser->target_count < parser->target_max, "Request contains too many target listeners: %d > %d", parser->target_count, parser->target_max);
        parser->targets[parser->target_count++] = strtoul(mark, NULL, 10); 
    }
#line 73 "src/handler_parser.rl"
	{
        check(p-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(p-targets_start), (int)target_expected_len);
    }
	goto st6;
tr16:
#line 65 "src/handler_parser.rl"
	{
        parser->extended = 1;
    }
#line 73 "src/handler_parser.rl"
	{
        check(p-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(p-targets_start), (int)target_expected_len);
    }
	goto st6;
st6:
	p += 1;
case 6:
#line 209 "src/handler_parser.c"
	if ( (*p) == 32 )
		goto tr10;
	goto st0;
tr10:
#line 79 "src/handler_parser.rl"
	{ {p++; cs = 11; goto _out;} }
	goto st11;
st11:
	p += 1;
case 11:
#line 220 "src/handler_parser.c"
	goto st0;
tr8:
#line 69 "src/handler_parser.rl"
	{
        targets_start = p;
    }
#line 48 "src/handler_parser.rl"
	{ mark = p; }
	goto st7;
tr14:
#line 60 "src/handler_parser.rl"
	{
        check(parser->target_count < parser->target_max, "Request contains too many target listeners: %d > %d", parser->target_count, parser->target_max);
        parser->targets[parser->target_count++] = strtoul(mark, NULL, 10); 
    }
#line 48 "src/handler_parser.rl"
	{ mark = p; }
	goto st7;
tr17:
#line 65 "src/handler_parser.rl"
	{
        parser->extended = 1;
    }
#line 48 "src/handler_parser.rl"
	{ mark = p; }
	goto st7;
st7:
	p += 1;
case 7:
#line 250 "src/handler_parser.c"
	switch( (*p) ) {
		case 32: goto st8;
		case 44: goto tr12;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st7;
	goto st0;
st8:
	p += 1;
case 8:
	if ( (*p) == 44 )
		goto tr12;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr14;
	goto st0;
tr9:
#line 69 "src/handler_parser.rl"
	{
        targets_start = p;
    }
	goto st9;
st9:
	p += 1;
case 9:
#line 275 "src/handler_parser.c"
	if ( (*p) == 32 )
		goto st10;
	goto st0;
st10:
	p += 1;
case 10:
	if ( (*p) == 44 )
		goto tr16;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr17;
	goto st0;
	}

	_out: {}
	}

#line 110 "src/handler_parser.rl"

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", 
            (long int)(pe - p));

    parser->body = blk2bstr(p, pe - p);

    if ( cs == 
#line 300 "src/handler_parser.c"
0
#line 116 "src/handler_parser.rl"
 ) {
        return -1;
    } else if ( cs >= 
#line 306 "src/handler_parser.c"
11
#line 118 "src/handler_parser.rl"
 ) {
        return 1;
    } else {
        return 0;
    }

error:
    return -1;
}


HandlerParser *HandlerParser_create(size_t max_targets)
{
    HandlerParser *parser = h_calloc(sizeof(HandlerParser), 1);
    check_mem(parser);

    parser->target_max = max_targets;
    parser->targets = h_calloc(sizeof(unsigned long), max_targets);
    check_mem(parser->targets);
    hattach(parser->targets, parser);

    return parser;

error:
    return NULL;
}

void HandlerParser_reset(HandlerParser *parser)
{
    if(parser->uuid) {
        bdestroy(parser->uuid);
        parser->uuid = NULL;
    }

    if(parser->body) {
        bdestroy(parser->body);
        parser->body = NULL;
    }
    parser->extended = 0;
}

void HandlerParser_destroy(HandlerParser *parser)
{
    if(parser != NULL) {
        HandlerParser_reset(parser);
        h_free(parser);
    }
}

