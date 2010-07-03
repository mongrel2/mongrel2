
#line 1 "src/state.rl"
#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>

#define CALL(A, C) if(state->actions && state->actions->A) next = state->actions->A(state, C, data)


#line 53 "src/state.rl"



#line 2 "src/state.c"
static const int StateActions_start = 17;
static const int StateActions_first_final = 17;
static const int StateActions_error = 0;

static const int StateActions_en_Proxy = 9;
static const int StateActions_en_main = 17;
static const int StateActions_en_main_Connection_Idle = 2;
static const int StateActions_en_main_Connection_HTTPRouting = 4;


#line 56 "src/state.rl"

int State_init(State *state, StateActions *actions)
{
    state->actions = actions;

    
#line 2 "src/state.c"
	{
	 state->cs = StateActions_start;
	}

#line 62 "src/state.rl"
    return 1;
}

inline int State_invariant(State *state, int event)
{
    if ( state->cs == 
#line 2 "src/state.c"
0
#line 67 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 2 "src/state.c"
17
#line 71 "src/state.rl"
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

    
#line 2 "src/state.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  state->cs )
	{
case 17:
	if ( (*p) == 110 )
		goto tr23;
	goto st0;
tr0:
#line 18 "src/state.rl"
	{ CALL(error, (*p)); }
	goto st0;
#line 2 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr23:
#line 17 "src/state.rl"
	{ CALL(open, (*p)); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 2 "src/state.c"
	if ( (*p) == 101 )
		goto tr1;
	goto tr0;
tr1:
#line 21 "src/state.rl"
	{ CALL(parse, (*p)); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr2;
		case 113: goto tr3;
	}
	goto tr0;
tr2:
#line 20 "src/state.rl"
	{ CALL(close, (*p)); }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 2 "src/state.c"
	if ( (*p) == 110 )
		goto tr23;
	goto tr0;
tr3:
#line 22 "src/state.rl"
	{ CALL(identify_request, (*p)); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 107: goto tr4;
		case 108: goto tr5;
		case 117: goto tr6;
	}
	goto tr0;
tr4:
#line 24 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 104: goto tr7;
		case 106: goto tr8;
		case 111: goto tr9;
	}
	goto tr0;
tr6:
#line 23 "src/state.rl"
	{ CALL(send_socket_response, (*p)); }
	goto st5;
tr7:
#line 30 "src/state.rl"
	{ CALL(http_to_directory, (*p)); }
	goto st5;
tr10:
#line 26 "src/state.rl"
	{ CALL(msg_to_directory, (*p)); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 2 "src/state.c"
	if ( (*p) == 116 )
		goto tr1;
	goto tr0;
tr8:
#line 28 "src/state.rl"
	{ CALL(http_to_handler, (*p)); }
	goto st6;
tr11:
#line 25 "src/state.rl"
	{ CALL(msg_to_handler, (*p)); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 2 "src/state.c"
	if ( (*p) == 114 )
		goto tr1;
	goto tr0;
tr9:
#line 29 "src/state.rl"
	{ CALL(http_to_proxy, (*p)); {goto st9;} }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 2 "src/state.c"
	goto tr0;
tr5:
#line 24 "src/state.rl"
	{ CALL(route_request, (*p)); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 104: goto tr10;
		case 106: goto tr11;
	}
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 103: goto tr12;
		case 105: goto tr14;
		case 112: goto tr15;
	}
	goto st0;
tr12:
#line 33 "src/state.rl"
	{ CALL(proxy_connected, (*p)); }
	goto st10;
tr22:
#line 35 "src/state.rl"
	{ CALL(proxy_send_request, (*p)); }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 2 "src/state.c"
	if ( (*p) == 114 )
		goto tr16;
	goto tr0;
tr16:
#line 36 "src/state.rl"
	{ CALL(proxy_read_response, (*p)); }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 2 "src/state.c"
	if ( (*p) == 115 )
		goto tr17;
	goto tr0;
tr17:
#line 37 "src/state.rl"
	{ CALL(proxy_send_response, (*p)); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 2 "src/state.c"
	if ( (*p) == 116 )
		goto tr18;
	goto tr0;
tr18:
#line 38 "src/state.rl"
	{ CALL(proxy_parse, (*p)); }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 112: goto tr15;
		case 113: goto st15;
	}
	goto tr0;
tr15:
#line 42 "src/state.rl"
	{
        CALL(proxy_exit_idle, (*p));
        {goto st2;} 
    }
	goto st14;
tr21:
#line 46 "src/state.rl"
	{
        CALL(proxy_exit_routing, (*p));
        p--;
        {goto st4;} 
    }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 2 "src/state.c"
	goto tr0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 107 )
		goto st16;
	goto tr0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 104: goto tr21;
		case 106: goto tr21;
		case 111: goto tr22;
	}
	goto tr0;
tr14:
#line 34 "src/state.rl"
	{ CALL(proxy_failed, (*p)); }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 2 "src/state.c"
	goto tr0;
	}
	_test_eof1:  state->cs = 1; goto _test_eof; 
	_test_eof2:  state->cs = 2; goto _test_eof; 
	_test_eof18:  state->cs = 18; goto _test_eof; 
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
	_test_eof16:  state->cs = 16; goto _test_eof; 
	_test_eof19:  state->cs = 19; goto _test_eof; 

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
	case 15: 
	case 16: 
#line 18 "src/state.rl"
	{ CALL(error, (*p)); }
	break;
	case 18: 
#line 19 "src/state.rl"
	{ CALL(finish, (*p)); }
	break;
#line 2 "src/state.c"
	}
	}

	_out: {}
	}

#line 89 "src/state.rl"

    return next;
}

int State_finish(State *state)
{
    return State_invariant(state, 0);
}


/* Do not access these directly or alter their order EVER.  */
const char *EVENT_NAMES[] = {
    "ACCEPT",
    "CLOSE",
    "CONNECT",
    "DIRECTORY",
    "FAILED",
    "HANDLER",
    "HTTP_REQ",
    "MSG_REQ",
    "MSG_RESP",
    "OPEN",
    "PROXY",
    "REMOTE_CLOSE",
    "REQ_RECV",
    "REQ_SENT",
    "RESP_RECV",
    "RESP_SENT",
    "SOCKET_REQ",
    "TIMEOUT"};

const char *State_event_name(int event)
{
    // TODO: find out why the hell the FSM is actually giving us this

    if(event == 0) {
        return "NUL";
    }

    assert(event > EVENT_START && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - EVENT_START - 1];
}
