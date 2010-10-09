
#line 1 "src/state.rl"
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

#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>

#define CALL(A, C) TRACE(A,C); if(state->actions && state->actions->A) next = state->actions->A(conn)


#line 86 "src/state.rl"



#line 52 "src/state.c"
static const int StateActions_start = 16;
static const int StateActions_first_final = 16;
static const int StateActions_error = 0;

static const int StateActions_en_Proxy = 10;
static const int StateActions_en_main = 16;
static const int StateActions_en_main_Connection_Idle = 5;
static const int StateActions_en_main_Connection_HTTPRouting = 3;


#line 89 "src/state.rl"

int State_init(State *state, StateActions *actions)
{
    state->actions = actions;

    
#line 70 "src/state.c"
	{
	 state->cs = StateActions_start;
	}

#line 95 "src/state.rl"
    return 1;
}

int State_invariant(State *state)
{
    if ( state->cs == 
#line 82 "src/state.c"
0
#line 100 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 90 "src/state.c"
16
#line 104 "src/state.rl"
 ) {
        return 1;
    }

    return 0;
}

int State_exec(State *state, int event, struct Connection *conn)
{
    int event_queue[2] = {0};
    event_queue[0] = event;
    int next = 0;

    const int *p = event_queue;
    const int *pe = p+1;
    const int *eof = event == CLOSE ? pe : NULL;

    
#line 111 "src/state.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  state->cs )
	{
case 16:
	if ( (*p) == 107 )
		goto tr20;
	goto st0;
tr0:
#line 51 "src/state.rl"
	{ CALL(error, (*p)); }
	goto st0;
#line 125 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr20:
#line 53 "src/state.rl"
	{ CALL(parse, (*p)); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 137 "src/state.c"
	switch( (*p) ) {
		case 100: goto tr1;
		case 110: goto tr2;
	}
	goto tr0;
tr1:
#line 52 "src/state.rl"
	{ CALL(close, (*p)); }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 151 "src/state.c"
	if ( (*p) == 107 )
		goto tr20;
	goto tr0;
tr2:
#line 54 "src/state.rl"
	{ CALL(register_request, (*p)); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 163 "src/state.c"
	switch( (*p) ) {
		case 100: goto tr1;
		case 105: goto tr3;
		case 106: goto tr4;
		case 113: goto tr5;
	}
	goto tr0;
tr3:
#line 57 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 179 "src/state.c"
	switch( (*p) ) {
		case 100: goto tr1;
		case 102: goto tr6;
		case 104: goto tr7;
		case 108: goto tr8;
	}
	goto tr0;
tr5:
#line 56 "src/state.rl"
	{ CALL(send_socket_response, (*p)); }
	goto st4;
tr6:
#line 62 "src/state.rl"
	{ CALL(http_to_directory, (*p)); }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 199 "src/state.c"
	switch( (*p) ) {
		case 100: goto tr1;
		case 112: goto tr9;
	}
	goto tr0;
tr9:
#line 53 "src/state.rl"
	{ CALL(parse, (*p)); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 213 "src/state.c"
	switch( (*p) ) {
		case 100: goto tr1;
		case 110: goto tr10;
	}
	goto tr0;
tr10:
#line 55 "src/state.rl"
	{ CALL(identify_request, (*p)); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 227 "src/state.c"
	switch( (*p) ) {
		case 105: goto tr3;
		case 106: goto tr4;
		case 113: goto tr5;
	}
	goto tr0;
tr4:
#line 57 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 242 "src/state.c"
	if ( (*p) == 104 )
		goto tr11;
	goto tr0;
tr7:
#line 60 "src/state.rl"
	{ CALL(http_to_handler, (*p)); }
	goto st8;
tr11:
#line 58 "src/state.rl"
	{ CALL(msg_to_handler, (*p)); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 258 "src/state.c"
	if ( (*p) == 111 )
		goto tr9;
	goto tr0;
tr8:
#line 61 "src/state.rl"
	{ CALL(http_to_proxy, (*p)); {goto st10;} }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 270 "src/state.c"
	goto tr0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 101: goto tr12;
		case 103: goto tr14;
	}
	goto st0;
tr12:
#line 65 "src/state.rl"
	{ CALL(proxy_deliver, (*p)); }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 289 "src/state.c"
	switch( (*p) ) {
		case 109: goto tr15;
		case 111: goto tr16;
	}
	goto tr0;
tr14:
#line 66 "src/state.rl"
	{ CALL(proxy_failed, (*p)); }
	goto st12;
tr15:
#line 71 "src/state.rl"
	{ CALL(proxy_close, (*p)); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 307 "src/state.c"
	if ( (*p) == 100 )
		goto tr17;
	goto tr0;
tr17:
#line 75 "src/state.rl"
	{
        p--;
        {goto st5;} 
    }
	goto st13;
tr19:
#line 79 "src/state.rl"
	{
        CALL(proxy_close, (*p));
        p--;
        {goto st3;} 
    }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 330 "src/state.c"
	goto tr0;
tr16:
#line 69 "src/state.rl"
	{ CALL(proxy_reply_parse, (*p)); }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 340 "src/state.c"
	switch( (*p) ) {
		case 109: goto tr15;
		case 110: goto tr18;
	}
	goto tr0;
tr18:
#line 70 "src/state.rl"
	{ CALL(proxy_req_parse, (*p)); }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 354 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr19;
		case 104: goto tr19;
		case 105: goto tr12;
		case 108: goto tr19;
		case 109: goto tr15;
	}
	goto tr0;
	}
	_test_eof1:  state->cs = 1; goto _test_eof; 
	_test_eof17:  state->cs = 17; goto _test_eof; 
	_test_eof2:  state->cs = 2; goto _test_eof; 
	_test_eof3:  state->cs = 3; goto _test_eof; 
	_test_eof4:  state->cs = 4; goto _test_eof; 
	_test_eof5:  state->cs = 5; goto _test_eof; 
	_test_eof6:  state->cs = 6; goto _test_eof; 
	_test_eof7:  state->cs = 7; goto _test_eof; 
	_test_eof8:  state->cs = 8; goto _test_eof; 
	_test_eof9:  state->cs = 9; goto _test_eof; 
	_test_eof10:  state->cs = 10; goto _test_eof; 
	_test_eof11:  state->cs = 11; goto _test_eof; 
	_test_eof12:  state->cs = 12; goto _test_eof; 
	_test_eof13:  state->cs = 13; goto _test_eof; 
	_test_eof14:  state->cs = 14; goto _test_eof; 
	_test_eof15:  state->cs = 15; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch (  state->cs ) {
	case 1: 
	case 2: 
	case 3: 
	case 4: 
	case 5: 
	case 6: 
	case 7: 
	case 8: 
	case 9: 
	case 11: 
	case 12: 
	case 13: 
	case 14: 
	case 15: 
#line 51 "src/state.rl"
	{ CALL(error, (*p)); }
	break;
#line 402 "src/state.c"
	}
	}

	_out: {}
	}

#line 122 "src/state.rl"

    return next;
}


/* Do not access these directly or alter their order EVER.  */
const char *EVENT_NAMES[] = {
    "CLOSE",
    "CONNECT",
    "DIRECTORY",
    "FAILED",
    "HANDLER",
    "HTTP_REQ",
    "MSG_REQ",
    "OPEN",
    "PROXY",
    "REMOTE_CLOSE",
    "REQ_RECV",
    "REQ_SENT",
    "RESP_SENT",
    "SOCKET_REQ",
    "TIMEOUT"};

const char *State_event_name(int event)
{
    if(event == 0) event = CLOSE;

    assert(event >= CLOSE && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - CLOSE];
}
