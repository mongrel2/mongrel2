
#line 1 "src/state.rl"
#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>



#line 111 "src/state.rl"



#line 2 "src/state.c"
static const int State_start = 19;
static const int State_first_final = 19;
static const int State_error = 0;

static const int State_en_Proxy = 11;
static const int State_en_main = 19;
static const int State_en_main_Connection_Idle = 2;
static const int State_en_main_Connection_HTTPRouting = 5;


#line 114 "src/state.rl"

int State_init(State *state)
{
    
#line 2 "src/state.c"
	{
	 state->cs = State_start;
	}

#line 118 "src/state.rl"
    return 1;
}

inline int State_invariant(State *state, int event)
{
    if ( state->cs == 
#line 2 "src/state.c"
0
#line 123 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 2 "src/state.c"
19
#line 127 "src/state.rl"
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
		goto tr26;
	goto st0;
tr0:
#line 16 "src/state.rl"
	{ debug("ERROR! "); }
	goto st0;
#line 2 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr26:
#line 14 "src/state.rl"
	{ debug("BEGIN "); }
#line 15 "src/state.rl"
	{ debug("OPEN "); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 2 "src/state.c"
	if ( (*p) == 101 )
		goto st2;
	goto tr0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 102: goto tr2;
		case 109: goto st3;
		case 114: goto st4;
		case 119: goto tr5;
	}
	goto tr0;
tr2:
#line 18 "src/state.rl"
	{ debug("CLOSE "); }
	goto st20;
tr5:
#line 19 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 2 "src/state.c"
	if ( (*p) == 110 )
		goto tr26;
	goto tr0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 117: goto st2;
		case 119: goto tr5;
	}
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 107: goto st5;
		case 108: goto st8;
		case 118: goto tr2;
	}
	goto tr0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 104: goto st3;
		case 106: goto st6;
		case 112: goto tr9;
	}
	goto tr0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 115 )
		goto st2;
	goto tr0;
tr9:
#line 87 "src/state.rl"
	{ {goto st11;} }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 2 "src/state.c"
	goto tr0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( (*p) ) {
		case 104: goto st3;
		case 106: goto st6;
		case 112: goto st9;
	}
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 115: goto st10;
		case 119: goto tr5;
	}
	goto tr0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 116: goto st3;
		case 119: goto tr5;
	}
	goto tr0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 103: goto tr12;
		case 105: goto tr14;
		case 113: goto tr15;
		case 119: goto tr16;
	}
	goto st0;
tr12:
#line 14 "src/state.rl"
	{ debug("BEGIN "); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 2 "src/state.c"
	switch( (*p) ) {
		case 115: goto st13;
		case 119: goto tr18;
	}
	goto tr0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( (*p) ) {
		case 116: goto st14;
		case 119: goto tr18;
	}
	goto tr0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( (*p) ) {
		case 117: goto st15;
		case 119: goto tr18;
	}
	goto tr0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 113: goto tr21;
		case 114: goto st17;
		case 119: goto tr18;
	}
	goto tr0;
tr15:
#line 14 "src/state.rl"
	{ debug("BEGIN "); }
#line 22 "src/state.rl"
	{{goto st2;} }
	goto st16;
tr21:
#line 22 "src/state.rl"
	{{goto st2;} }
	goto st16;
tr24:
#line 23 "src/state.rl"
	{ p--; {goto st5;} }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 2 "src/state.c"
	goto tr0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 107 )
		goto st18;
	goto tr0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( (*p) ) {
		case 104: goto tr24;
		case 106: goto tr24;
		case 112: goto st12;
	}
	goto tr0;
tr18:
#line 19 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st21;
tr14:
#line 14 "src/state.rl"
	{ debug("BEGIN "); }
	goto st21;
tr16:
#line 14 "src/state.rl"
	{ debug("BEGIN "); }
#line 19 "src/state.rl"
	{ debug("TIMEOUT"); }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 2 "src/state.c"
	goto tr0;
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
	case 12: 
	case 13: 
	case 14: 
	case 15: 
	case 16: 
	case 17: 
	case 18: 
#line 16 "src/state.rl"
	{ debug("ERROR! "); }
	break;
	case 20: 
	case 21: 
#line 17 "src/state.rl"
	{ debug("FINISH "); }
	break;
#line 2 "src/state.c"
	}
	}

	_out: {}
	}

#line 141 "src/state.rl"

    return State_invariant(state, event);
}

int State_finish(State *state)
{
    return State_invariant(state, 0);
}


