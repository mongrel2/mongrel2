
#line 1 "src/state.rl"
#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>



#line 81 "src/state.rl"



#line 2 "src/state.c"
static const int State_start = 18;
static const int State_first_final = 18;
static const int State_error = 0;

static const int State_en_Proxy = 11;
static const int State_en_main = 18;
static const int State_en_main_Connection_Idle = 2;
static const int State_en_main_Connection_HTTPRouting = 9;


#line 84 "src/state.rl"

int State_init(State *state)
{
    
#line 2 "src/state.c"
	{
	 state->cs = State_start;
	}

#line 88 "src/state.rl"
    return 1;
}

inline int State_invariant(State *state, int event)
{
    if ( state->cs == 
#line 2 "src/state.c"
0
#line 93 "src/state.rl"
 ) {
        return -1;
    }

    if ( state->cs >= 
#line 2 "src/state.c"
18
#line 97 "src/state.rl"
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
case 18:
	if ( (*p) == 1 )
		goto tr22;
	goto st0;
tr0:
#line 15 "src/state.rl"
	{ debug("ERROR! "); }
	goto st0;
#line 2 "src/state.c"
st0:
 state->cs = 0;
	goto _out;
tr22:
#line 13 "src/state.rl"
	{ debug("BEGIN "); }
#line 14 "src/state.rl"
	{ debug("OPEN "); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 2 "src/state.c"
	if ( (*p) == 7 )
		goto st2;
	goto tr0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 2: goto tr2;
		case 3: goto st3;
		case 4: goto st6;
		case 9: goto st2;
	}
	goto tr0;
tr2:
#line 17 "src/state.rl"
	{ debug("CLOSE "); }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 2 "src/state.c"
	if ( (*p) == 1 )
		goto tr22;
	goto tr0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 8: goto st4;
		case 10: goto st9;
	}
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 14: goto st5;
		case 15: goto st6;
		case 16: goto st7;
	}
	goto tr0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 5 )
		goto st2;
	goto tr0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 6 )
		goto st2;
	goto tr0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 5 )
		goto st8;
	goto tr0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 4 )
		goto st6;
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 14: goto st5;
		case 15: goto st6;
		case 16: goto tr10;
	}
	goto tr0;
tr10:
#line 66 "src/state.rl"
	{ {goto st11;} }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 2 "src/state.c"
	goto tr0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 11: goto tr11;
		case 12: goto tr13;
		case 17: goto tr14;
	}
	goto st0;
tr11:
#line 13 "src/state.rl"
	{ debug("BEGIN "); }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 2 "src/state.c"
	goto tr0;
tr13:
#line 13 "src/state.rl"
	{ debug("BEGIN "); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 2 "src/state.c"
	if ( (*p) == 5 )
		goto st13;
	goto tr0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 4 )
		goto st14;
	goto tr0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 6 )
		goto st15;
	goto tr0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 10: goto st16;
		case 17: goto tr19;
	}
	goto tr0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( (*p) == 16 )
		goto st12;
	if ( 14 <= (*p) && (*p) <= 15 )
		goto tr20;
	goto tr0;
tr14:
#line 13 "src/state.rl"
	{ debug("BEGIN "); }
#line 20 "src/state.rl"
	{{goto st2;} }
	goto st17;
tr19:
#line 20 "src/state.rl"
	{{goto st2;} }
	goto st17;
tr20:
#line 21 "src/state.rl"
	{ p--; {goto st9;} }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 2 "src/state.c"
	goto tr0;
	}
	_test_eof1:  state->cs = 1; goto _test_eof; 
	_test_eof2:  state->cs = 2; goto _test_eof; 
	_test_eof19:  state->cs = 19; goto _test_eof; 
	_test_eof3:  state->cs = 3; goto _test_eof; 
	_test_eof4:  state->cs = 4; goto _test_eof; 
	_test_eof5:  state->cs = 5; goto _test_eof; 
	_test_eof6:  state->cs = 6; goto _test_eof; 
	_test_eof7:  state->cs = 7; goto _test_eof; 
	_test_eof8:  state->cs = 8; goto _test_eof; 
	_test_eof9:  state->cs = 9; goto _test_eof; 
	_test_eof10:  state->cs = 10; goto _test_eof; 
	_test_eof11:  state->cs = 11; goto _test_eof; 
	_test_eof20:  state->cs = 20; goto _test_eof; 
	_test_eof12:  state->cs = 12; goto _test_eof; 
	_test_eof13:  state->cs = 13; goto _test_eof; 
	_test_eof14:  state->cs = 14; goto _test_eof; 
	_test_eof15:  state->cs = 15; goto _test_eof; 
	_test_eof16:  state->cs = 16; goto _test_eof; 
	_test_eof17:  state->cs = 17; goto _test_eof; 

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
#line 15 "src/state.rl"
	{ debug("ERROR! "); }
	break;
	case 19: 
	case 20: 
#line 16 "src/state.rl"
	{ debug("FINISH "); }
	break;
#line 2 "src/state.c"
	}
	}

	_out: {}
	}

#line 111 "src/state.rl"

    return State_invariant(state, event);
}

int State_finish(State *state)
{
    return State_invariant(state, 0);
}


