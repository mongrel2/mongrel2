
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

#define CALL(A, C) if(state->actions && state->actions->A) next = state->actions->A(C, data)


#line 87 "src/state.rl"



#line 52 "src/state.c"
static const int StateActions_start = 15;
static const int StateActions_first_final = 15;
static const int StateActions_error = 0;

static const int StateActions_en_Proxy = 9;
static const int StateActions_en_main = 15;
static const int StateActions_en_main_Connection_Idle = 2;
static const int StateActions_en_main_Connection_HTTPRouting = 4;


#line 90 "src/state.rl"

int State_init(State *state, StateActions *actions)
{
    state->actions = actions;

    
#line 70 "src/state.c"
	{
	 state->cs = StateActions_start;
	}

#line 96 "src/state.rl"
    return 1;
}

static inline int State_invariant(State *state, int event)
{
    if ( state->cs == 
#line 82 "src/state.c"
0
#line 101 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 90 "src/state.c"
15
#line 105 "src/state.rl"
 ) {
        return 1;
    }

    return 0;
}

int State_exec(State *state, int event, void *data)
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
case 15:
	if ( (*p) == 109 )
		goto tr19;
	goto st0;
tr0:
#line 52 "src/state.rl"
	{ CALL(error, (*p)); }
	goto st0;
#line 125 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr19:
#line 51 "src/state.rl"
	{ CALL(open, (*p)); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 137 "src/state.c"
	if ( (*p) == 101 )
		goto tr1;
	goto tr0;
tr1:
#line 55 "src/state.rl"
	{ CALL(parse, (*p)); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 149 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr2;
		case 112: goto tr3;
	}
	goto tr0;
tr2:
#line 54 "src/state.rl"
	{ CALL(close, (*p)); }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 163 "src/state.c"
	if ( (*p) == 109 )
		goto tr19;
	goto tr0;
tr3:
#line 56 "src/state.rl"
	{ CALL(identify_request, (*p)); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 175 "src/state.c"
	switch( (*p) ) {
		case 107: goto tr4;
		case 108: goto tr5;
		case 115: goto tr6;
	}
	goto tr0;
tr4:
#line 58 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 190 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr2;
		case 104: goto tr7;
		case 106: goto tr8;
		case 110: goto tr9;
	}
	goto tr0;
tr6:
#line 57 "src/state.rl"
	{ CALL(send_socket_response, (*p)); }
	goto st5;
tr7:
#line 63 "src/state.rl"
	{ CALL(http_to_directory, (*p)); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 210 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr2;
		case 114: goto tr1;
	}
	goto tr0;
tr8:
#line 61 "src/state.rl"
	{ CALL(http_to_handler, (*p)); }
	goto st6;
tr10:
#line 59 "src/state.rl"
	{ CALL(msg_to_handler, (*p)); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 228 "src/state.c"
	if ( (*p) == 113 )
		goto tr1;
	goto tr0;
tr9:
#line 62 "src/state.rl"
	{ CALL(http_to_proxy, (*p)); {goto st9;} }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 240 "src/state.c"
	goto tr0;
tr5:
#line 58 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 250 "src/state.c"
	if ( (*p) == 106 )
		goto tr10;
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 103: goto tr11;
		case 105: goto tr13;
	}
	goto st0;
tr11:
#line 66 "src/state.rl"
	{ CALL(proxy_deliver, (*p)); }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 271 "src/state.c"
	switch( (*p) ) {
		case 111: goto tr14;
		case 113: goto tr15;
	}
	goto tr0;
tr13:
#line 67 "src/state.rl"
	{ CALL(proxy_failed, (*p)); }
	goto st11;
tr14:
#line 72 "src/state.rl"
	{ CALL(proxy_close, (*p)); }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 289 "src/state.c"
	if ( (*p) == 102 )
		goto tr16;
	goto tr0;
tr16:
#line 76 "src/state.rl"
	{
        p--;
        {goto st2;} 
    }
	goto st12;
tr18:
#line 80 "src/state.rl"
	{
        CALL(proxy_close, (*p));
        p--;
        {goto st4;} 
    }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 312 "src/state.c"
	goto tr0;
tr15:
#line 70 "src/state.rl"
	{ CALL(proxy_reply_parse, (*p)); }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 322 "src/state.c"
	switch( (*p) ) {
		case 111: goto tr14;
		case 112: goto tr17;
	}
	goto tr0;
tr17:
#line 71 "src/state.rl"
	{ CALL(proxy_req_parse, (*p)); }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 336 "src/state.c"
	switch( (*p) ) {
		case 104: goto tr18;
		case 106: goto tr18;
		case 107: goto tr11;
		case 110: goto tr18;
		case 111: goto tr14;
	}
	goto tr0;
	}
	_test_eof1:  state->cs = 1; goto _test_eof; 
	_test_eof2:  state->cs = 2; goto _test_eof; 
	_test_eof16:  state->cs = 16; goto _test_eof; 
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
	case 10: 
	case 11: 
	case 12: 
	case 13: 
	case 14: 
#line 52 "src/state.rl"
	{ CALL(error, (*p)); }
	break;
	case 16: 
#line 53 "src/state.rl"
	{ CALL(finish, (*p)); }
	break;
#line 386 "src/state.c"
	}
	}

	_out: {}
	}

#line 123 "src/state.rl"

    return next;
}

int State_finish(State *state)
{
    return State_invariant(state, 0);
}


/* Do not access these directly or alter their order EVER.  */
const char *EVENT_NAMES[] = {
    "FINISHED",
    "ACCEPT",
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
    if(event == 0) event = FINISHED;

    assert(event >= FINISHED && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - FINISHED];
}
