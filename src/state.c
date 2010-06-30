
#line 1 "src/state.rl"
#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>



#line 46 "src/state.rl"



#line 2 "src/state.c"
static const char _State_actions[] = {
	0, 1, 2, 1, 3, 1, 5, 1, 
	6, 1, 7, 1, 8, 1, 9, 2, 
	0, 1, 2, 4, 10, 4, 4, 10, 
	0, 1
};

static const char _State_key_offsets[] = {
	0, 0, 1, 4, 5, 6, 7
};

static const char _State_trans_keys[] = {
	7, 2, 3, 4, 5, 6, 1, 1, 
	0
};

static const char _State_single_lengths[] = {
	0, 1, 3, 1, 1, 1, 1
};

static const char _State_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0
};

static const char _State_index_offsets[] = {
	0, 0, 2, 6, 8, 10, 12
};

static const char _State_trans_targs[] = {
	2, 0, 6, 3, 4, 0, 2, 0, 
	2, 0, 1, 0, 1, 0, 0
};

static const char _State_trans_actions[] = {
	1, 3, 5, 7, 11, 3, 13, 3, 
	9, 3, 15, 0, 21, 3, 0
};

static const char _State_eof_actions[] = {
	0, 3, 3, 3, 3, 0, 18
};

static const int State_start = 5;
static const int State_first_final = 5;
static const int State_error = 0;

static const int State_en_main = 5;


#line 49 "src/state.rl"

int State_init(State *state)
{
  
#line 2 "src/state.c"
	{
	 state->cs = State_start;
	}

#line 53 "src/state.rl"
  return 1;
}

inline int State_invariant(State *state, int event)
{
  if ( state->cs == State_error ) {
    return -1;
  }

  if ( state->cs >= State_first_final )
    return 1;
  
  return 0;
}

int State_exec(State *state, int event)
{
  const char *p = (const char *)&event;
  const char *pe = p+1;
  const char *eof = NULL;

  
#line 2 "src/state.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if (  state->cs == 0 )
		goto _out;
_resume:
	_keys = _State_trans_keys + _State_key_offsets[ state->cs];
	_trans = _State_index_offsets[ state->cs];

	_klen = _State_single_lengths[ state->cs];
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

	_klen = _State_range_lengths[ state->cs];
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
	 state->cs = _State_trans_targs[_trans];

	if ( _State_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _State_actions + _State_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 12 "src/state.rl"
	{ debug("BEGIN "); }
	break;
	case 1:
#line 13 "src/state.rl"
	{ debug("OPEN "); }
	break;
	case 2:
#line 14 "src/state.rl"
	{ debug("CONNECTED"); }
	break;
	case 3:
#line 15 "src/state.rl"
	{ debug("ERROR! "); }
	break;
	case 4:
#line 16 "src/state.rl"
	{ debug("FINISH "); }
	break;
	case 5:
#line 17 "src/state.rl"
	{ debug("CLOSE "); }
	break;
	case 6:
#line 18 "src/state.rl"
	{ debug("RECV "); }
	break;
	case 7:
#line 19 "src/state.rl"
	{ debug("SENT "); }
	break;
	case 8:
#line 20 "src/state.rl"
	{ debug("SERVICE"); }
	break;
	case 9:
#line 21 "src/state.rl"
	{ debug("DELIVERED "); }
	break;
	case 10:
#line 45 "src/state.rl"
	{ debug("\n"); }
	break;
#line 2 "src/state.c"
		}
	}

_again:
	if (  state->cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _State_actions + _State_eof_actions[ state->cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 3:
#line 15 "src/state.rl"
	{ debug("ERROR! "); }
	break;
	case 4:
#line 16 "src/state.rl"
	{ debug("FINISH "); }
	break;
	case 10:
#line 45 "src/state.rl"
	{ debug("\n"); }
	break;
#line 2 "src/state.c"
		}
	}
	}

	_out: {}
	}

#line 75 "src/state.rl"

  return State_invariant(state, event);
}

int State_finish(State *state)
{
  return State_invariant(state, 0);
}


