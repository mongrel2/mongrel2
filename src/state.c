
#line 1 "src/state.rl"
#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>



#line 52 "src/state.rl"



#line 2 "src/state.c"
static const int StateActions_start = 19;
static const int StateActions_first_final = 19;
static const int StateActions_error = 0;

static const int StateActions_en_Proxy = 11;
static const int StateActions_en_main = 19;
static const int StateActions_en_main_Connection_Idle = 2;
static const int StateActions_en_main_Connection_HTTPRouting = 5;


#line 55 "src/state.rl"

int State_init(State *state)
{
    
#line 2 "src/state.c"
	{
	 state->cs = StateActions_start;
	}

#line 59 "src/state.rl"
    return 1;
}

inline int State_invariant(State *state, int event)
{
    if ( state->cs == 
#line 2 "src/state.c"
0
#line 64 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 2 "src/state.c"
19
#line 68 "src/state.rl"
 ) {
        return 1;
    }

    return 0;
}

int State_exec(State *state, int event)
{
    const char *p = (const char *)&event;
    const char *pe = p+1;
    const char *eof = event == CLOSE ? pe : NULL;

    
#line 2 "src/state.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  state->cs )
	{
case 19:
	if ( (*p) == 110 )
		goto tr33;
	goto st0;
tr0:
#line 17 "src/state.rl"
	{ debug("ERROR! "); }
	goto st0;
tr23:
#line 49 "src/state.rl"
	{ debug("PROXY ERROR! "); }
	goto st0;
#line 2 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr33:
#line 15 "src/state.rl"
	{ debug("BEGIN "); }
#line 16 "src/state.rl"
	{ debug("OPEN "); }
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
	{ debug("ACCEPTED"); }
	goto st2;
tr6:
#line 33 "src/state.rl"
	{ debug("RESP SENT"); }
	goto st2;
tr12:
#line 32 "src/state.rl"
	{ debug("REQ SENT"); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 102: goto tr2;
		case 109: goto tr3;
		case 113: goto st4;
		case 118: goto tr5;
	}
	goto tr0;
tr2:
#line 19 "src/state.rl"
	{ debug("CLOSE "); }
	goto st20;
tr5:
#line 20 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 2 "src/state.c"
	if ( (*p) == 110 )
		goto tr33;
	goto tr0;
tr3:
#line 24 "src/state.rl"
	{ debug("MSG RESP"); }
	goto st3;
tr9:
#line 31 "src/state.rl"
	{ debug("HTTP TO DIRECTORY"); }
	goto st3;
tr13:
#line 27 "src/state.rl"
	{ debug("MSG TO DIRECTORY"); }
	goto st3;
tr17:
#line 34 "src/state.rl"
	{ debug("RESP RECV"); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 116: goto tr6;
		case 118: goto tr5;
	}
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 107: goto tr7;
		case 108: goto tr8;
		case 117: goto tr2;
	}
	goto tr0;
tr7:
#line 22 "src/state.rl"
	{ debug("HTTP REQUEST"); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 104: goto tr9;
		case 106: goto tr10;
		case 111: goto tr11;
	}
	goto tr0;
tr10:
#line 29 "src/state.rl"
	{ debug("HTTP TO HANDLER"); }
	goto st6;
tr14:
#line 25 "src/state.rl"
	{ debug("MSG TO HANDLER"); }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 2 "src/state.c"
	if ( (*p) == 114 )
		goto tr12;
	goto tr0;
tr11:
#line 30 "src/state.rl"
	{ debug("HTTP TO PROXY"); {goto st11;} }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 2 "src/state.c"
	goto tr0;
tr8:
#line 23 "src/state.rl"
	{ debug("MSG REQUEST"); }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 104: goto tr13;
		case 106: goto tr14;
		case 111: goto tr15;
	}
	goto tr0;
tr15:
#line 26 "src/state.rl"
	{ debug("MSG TO PROXY"); }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 114: goto tr16;
		case 118: goto tr5;
	}
	goto tr0;
tr16:
#line 32 "src/state.rl"
	{ debug("REQ SENT"); }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 115: goto tr17;
		case 118: goto tr5;
	}
	goto tr0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 103: goto tr18;
		case 105: goto tr20;
		case 112: goto tr21;
		case 118: goto tr22;
	}
	goto st0;
tr18:
#line 15 "src/state.rl"
	{ debug("BEGIN "); }
#line 37 "src/state.rl"
	{ debug("PROXY CONNECTED"); }
	goto st12;
tr32:
#line 39 "src/state.rl"
	{ debug("PROXY REQUEST"); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 114: goto tr24;
		case 118: goto tr25;
	}
	goto tr23;
tr24:
#line 40 "src/state.rl"
	{ debug("PROXY REQUEST SENT"); }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 115: goto tr26;
		case 118: goto tr25;
	}
	goto tr23;
tr26:
#line 42 "src/state.rl"
	{ debug("PROXY RESP RECV"); }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 116: goto tr27;
		case 118: goto tr25;
	}
	goto tr23;
tr27:
#line 41 "src/state.rl"
	{ debug("PROXY RESP SENT"); }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 112: goto tr28;
		case 113: goto st17;
		case 118: goto tr25;
	}
	goto tr23;
tr21:
#line 15 "src/state.rl"
	{ debug("BEGIN "); }
#line 46 "src/state.rl"
	{ debug("PROXY EXIT IDLE"); {goto st2;} }
	goto st16;
tr28:
#line 46 "src/state.rl"
	{ debug("PROXY EXIT IDLE"); {goto st2;} }
	goto st16;
tr31:
#line 47 "src/state.rl"
	{ debug("PROXY EXIT ROUTING"); p--; {goto st5;} }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 2 "src/state.c"
	goto tr23;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 107 )
		goto st18;
	goto tr23;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( (*p) ) {
		case 104: goto tr31;
		case 106: goto tr31;
		case 111: goto tr32;
	}
	goto tr23;
tr25:
#line 20 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st21;
tr20:
#line 15 "src/state.rl"
	{ debug("BEGIN "); }
#line 38 "src/state.rl"
	{ debug("PROXY FAILED"); }
	goto st21;
tr22:
#line 15 "src/state.rl"
	{ debug("BEGIN "); }
#line 20 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 2 "src/state.c"
	goto tr23;
	}
	_test_eof1:  state->cs = 1; goto _test_eof; 
	_test_eof2:  state->cs = 2; goto _test_eof; 
	_test_eof20:  state->cs = 20; goto _test_eof; 
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
	_test_eof17:  state->cs = 17; goto _test_eof; 
	_test_eof18:  state->cs = 18; goto _test_eof; 
	_test_eof21:  state->cs = 21; goto _test_eof; 

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
	case 10: 
#line 17 "src/state.rl"
	{ debug("ERROR! "); }
	break;
	case 20: 
#line 18 "src/state.rl"
	{ debug("FINISH "); }
	break;
	case 12: 
	case 13: 
	case 14: 
	case 15: 
	case 16: 
	case 17: 
	case 18: 
#line 49 "src/state.rl"
	{ debug("PROXY ERROR! "); }
	break;
#line 2 "src/state.c"
	}
	}

	_out: {}
	}

#line 82 "src/state.rl"

    return State_invariant(state, event);
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
    assert(event > EVENT_START && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - EVENT_START - 1];
}
