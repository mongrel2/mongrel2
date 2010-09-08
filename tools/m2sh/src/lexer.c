
#line 1 "src/lexer.rl"
#include "token.h"
#include "parser.h"

#include <stdio.h>

#include <bstring.h>
#include <dbg.h>
#include <adt/list.h>
#include <stdlib.h>


#define TK(N) debug("> " # N ": %.*s", (int)(te - ts), ts);\
    temp = calloc(sizeof(Token), 1);\
    temp->type = TK##N;\
    temp->data = blk2bstr(ts, (int)(te - ts));\
    list_append(token_list, lnode_create(temp)); 
     


#line 54 "src/lexer.rl"



#line 27 "src/lexer.c"
static const char _m2sh_lexer_key_offsets[] = {
	0, 0, 2, 2, 3, 5, 5, 7, 
	7, 29, 31, 38
};

static const char _m2sh_lexer_trans_keys[] = {
	34, 92, 10, 39, 92, 92, 96, 32, 
	34, 35, 39, 40, 41, 44, 58, 61, 
	91, 93, 96, 123, 125, 9, 13, 48, 
	57, 65, 90, 95, 122, 48, 57, 95, 
	48, 57, 65, 90, 97, 122, 95, 48, 
	57, 65, 90, 97, 122, 0
};

static const char _m2sh_lexer_single_lengths[] = {
	0, 2, 0, 1, 2, 0, 2, 0, 
	14, 0, 1, 1
};

static const char _m2sh_lexer_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	4, 1, 3, 3
};

static const char _m2sh_lexer_index_offsets[] = {
	0, 0, 3, 4, 6, 9, 10, 13, 
	14, 33, 35, 40
};

static const char _m2sh_lexer_trans_targs[] = {
	8, 2, 1, 1, 8, 3, 8, 5, 
	4, 4, 7, 8, 6, 6, 8, 1, 
	3, 4, 8, 8, 8, 8, 8, 8, 
	8, 6, 8, 8, 8, 9, 10, 11, 
	0, 9, 8, 11, 11, 10, 10, 8, 
	11, 11, 11, 11, 8, 8, 8, 8, 
	0
};

static const char _m2sh_lexer_trans_actions[] = {
	1, 0, 0, 0, 2, 0, 1, 0, 
	0, 0, 0, 3, 0, 0, 6, 0, 
	0, 0, 7, 8, 9, 10, 11, 13, 
	14, 0, 15, 16, 6, 0, 12, 0, 
	0, 0, 17, 0, 0, 19, 19, 18, 
	0, 0, 0, 0, 20, 17, 18, 20, 
	0
};

static const char _m2sh_lexer_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	4, 0, 0, 0
};

static const char _m2sh_lexer_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	5, 0, 0, 0
};

static const char _m2sh_lexer_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 46, 47, 48
};

static const int m2sh_lexer_start = 8;
static const int m2sh_lexer_first_final = 8;
static const int m2sh_lexer_error = 0;

static const int m2sh_lexer_en_main = 8;


#line 57 "src/lexer.rl"

list_t *Lexer_tokenize(bstring content) 
{
    list_t *token_list = list_create(LISTCOUNT_T_MAX);
    check_mem(token_list);
    Token *temp;

    char *p = bdata(content);
    char *pe = p + blength(content);
    char *eof = pe;
    int cs = -1;
    int act = -1;
    char *ts = NULL;
    char *te = NULL;

    
#line 116 "src/lexer.c"
	{
	cs = m2sh_lexer_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 73 "src/lexer.rl"
    
#line 126 "src/lexer.c"
	{
	int _klen;
	const char *_keys;
	int _trans;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( _m2sh_lexer_from_state_actions[cs] ) {
	case 5:
#line 1 "src/lexer.rl"
	{ts = p;}
	break;
#line 142 "src/lexer.c"
	}

	_keys = _m2sh_lexer_trans_keys + _m2sh_lexer_key_offsets[cs];
	_trans = _m2sh_lexer_index_offsets[cs];

	_klen = _m2sh_lexer_single_lengths[cs];
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

	_klen = _m2sh_lexer_range_lengths[cs];
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
_eof_trans:
	cs = _m2sh_lexer_trans_targs[_trans];

	if ( _m2sh_lexer_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _m2sh_lexer_trans_actions[_trans] ) {
	case 1:
#line 35 "src/lexer.rl"
	{te = p+1;{ TK(QSTRING) }}
	break;
	case 3:
#line 36 "src/lexer.rl"
	{te = p+1;{ TK(PATTERN) }}
	break;
	case 11:
#line 37 "src/lexer.rl"
	{te = p+1;{ TK(EQ) }}
	break;
	case 15:
#line 38 "src/lexer.rl"
	{te = p+1;{ TK(LBRACKET) }}
	break;
	case 16:
#line 39 "src/lexer.rl"
	{te = p+1;{ TK(RBRACKET) }}
	break;
	case 13:
#line 40 "src/lexer.rl"
	{te = p+1;{ TK(LBRACE) }}
	break;
	case 14:
#line 41 "src/lexer.rl"
	{te = p+1;{ TK(RBRACE) }}
	break;
	case 7:
#line 42 "src/lexer.rl"
	{te = p+1;{ TK(LPAREN) }}
	break;
	case 8:
#line 43 "src/lexer.rl"
	{te = p+1;{ TK(RPAREN) }}
	break;
	case 9:
#line 44 "src/lexer.rl"
	{te = p+1;{ TK(COMMA) }}
	break;
	case 10:
#line 45 "src/lexer.rl"
	{te = p+1;{ TK(COLON) }}
	break;
	case 6:
#line 47 "src/lexer.rl"
	{te = p+1;}
	break;
	case 2:
#line 48 "src/lexer.rl"
	{te = p+1;}
	break;
	case 17:
#line 50 "src/lexer.rl"
	{te = p;p--;{ TK(NUMBER) }}
	break;
	case 20:
#line 52 "src/lexer.rl"
	{te = p;p--;{ TK(IDENT) }}
	break;
	case 18:
#line 1 "src/lexer.rl"
	{	switch( act ) {
	case 15:
	{{p = ((te))-1;} TK(CLASS) }
	break;
	case 16:
	{{p = ((te))-1;} TK(IDENT) }
	break;
	}
	}
	break;
	case 19:
#line 1 "src/lexer.rl"
	{te = p+1;}
#line 51 "src/lexer.rl"
	{act = 15;}
	break;
	case 12:
#line 1 "src/lexer.rl"
	{te = p+1;}
#line 52 "src/lexer.rl"
	{act = 16;}
	break;
#line 285 "src/lexer.c"
	}

_again:
	switch ( _m2sh_lexer_to_state_actions[cs] ) {
	case 4:
#line 1 "src/lexer.rl"
	{ts = 0;}
	break;
#line 294 "src/lexer.c"
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _m2sh_lexer_eof_trans[cs] > 0 ) {
		_trans = _m2sh_lexer_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

#line 74 "src/lexer.rl"

    if ( cs == 
#line 316 "src/lexer.c"
0
#line 75 "src/lexer.rl"
 ) {
        debug("ERROR AT: %d\n%s", (int)(pe - p), p);
    } else if ( cs >= 
#line 322 "src/lexer.c"
8
#line 77 "src/lexer.rl"
 ) {
        debug("FINISHED");
    } else {
        debug("NOT FINISHED");
    }

    return token_list;

error:
    list_destroy(token_list);
    return NULL;
}
