
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

#include "handler_parser.h"
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <bstring.h>
#include <dbg.h>


#line 84 "src/handler_parser.rl"


/** Data **/

#line 51 "src/handler_parser.c"
static const int HandlerParser_start = 1;
static const int HandlerParser_first_final = 9;
static const int HandlerParser_error = 0;

static const int HandlerParser_en_main = 1;


#line 88 "src/handler_parser.rl"


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

    
#line 75 "src/handler_parser.c"
	{
	cs = HandlerParser_start;
	}

#line 103 "src/handler_parser.rl"
    
#line 82 "src/handler_parser.c"
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
#line 46 "src/handler_parser.rl"
	{ mark = p; }
	goto st2;
st2:
	p += 1;
case 2:
#line 108 "src/handler_parser.c"
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
#line 54 "src/handler_parser.rl"
	{
        parser->uuid = blk2bstr(mark, p-mark);
    }
	goto st3;
st3:
	p += 1;
case 3:
#line 131 "src/handler_parser.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr4;
	goto st0;
tr4:
#line 46 "src/handler_parser.rl"
	{ mark = p; }
	goto st4;
st4:
	p += 1;
case 4:
#line 142 "src/handler_parser.c"
	if ( (*p) == 58 )
		goto tr6;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st4;
	goto st0;
tr6:
#line 48 "src/handler_parser.rl"
	{
        char *endptr = NULL;
        target_expected_len = strtoul(mark, &endptr, 10);
        check(endptr == p, "Invalid length given, didn't parse correctly.");
    }
	goto st5;
st5:
	p += 1;
case 5:
#line 159 "src/handler_parser.c"
	if ( (*p) == 44 )
		goto tr7;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr8;
	goto st0;
tr7:
#line 63 "src/handler_parser.rl"
	{
        targets_start = p;
    }
#line 67 "src/handler_parser.rl"
	{
        check(p-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(p-targets_start), (int)target_expected_len);
    }
	goto st6;
tr11:
#line 58 "src/handler_parser.rl"
	{
        check(parser->target_count < MAX_TARGETS, "Request contains too many target listeners.");
        parser->targets[parser->target_count++] = strtoul(mark, NULL, 10); 
    }
#line 67 "src/handler_parser.rl"
	{
        check(p-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(p-targets_start), (int)target_expected_len);
    }
	goto st6;
st6:
	p += 1;
case 6:
#line 193 "src/handler_parser.c"
	if ( (*p) == 32 )
		goto tr9;
	goto st0;
tr9:
#line 73 "src/handler_parser.rl"
	{ {p++; cs = 9; goto _out;} }
	goto st9;
st9:
	p += 1;
case 9:
#line 204 "src/handler_parser.c"
	goto st0;
tr8:
#line 63 "src/handler_parser.rl"
	{
        targets_start = p;
    }
#line 46 "src/handler_parser.rl"
	{ mark = p; }
	goto st7;
tr13:
#line 58 "src/handler_parser.rl"
	{
        check(parser->target_count < MAX_TARGETS, "Request contains too many target listeners.");
        parser->targets[parser->target_count++] = strtoul(mark, NULL, 10); 
    }
#line 46 "src/handler_parser.rl"
	{ mark = p; }
	goto st7;
st7:
	p += 1;
case 7:
#line 226 "src/handler_parser.c"
	switch( (*p) ) {
		case 32: goto st8;
		case 44: goto tr11;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st7;
	goto st0;
st8:
	p += 1;
case 8:
	if ( (*p) == 44 )
		goto tr11;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr13;
	goto st0;
	}

	_out: {}
	}

#line 104 "src/handler_parser.rl"

    assert(p <= pe && "Buffer overflow after parsing.");

    parser->body_start = p;
    parser->body_length = pe - p;

    if ( cs == 
#line 255 "src/handler_parser.c"
0
#line 110 "src/handler_parser.rl"
 ) {
        return -1;
    } else if ( cs >= 
#line 261 "src/handler_parser.c"
9
#line 112 "src/handler_parser.rl"
 ) {
        return 1;
    } else {
        return 0;
    }

error:
    return -1;
}


