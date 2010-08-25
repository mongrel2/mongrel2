
#line 1 "src/kegogi_lexer.rl"
#include "kegogi_tokens.h"
#include "kegogi_parser.h"

#include <stdio.h>

#include <bstring.h>
#include <dbg.h>


#line 85 "src/kegogi_lexer.rl"



#line 17 "src/kegogi_lexer.c"
static const char _kegogi_lexer_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	6, 1, 21, 1, 22, 1, 23, 1, 
	27, 2, 0, 2, 2, 1, 2, 2, 
	1, 19, 2, 1, 20, 2, 1, 21, 
	2, 1, 24, 2, 1, 25, 2, 1, 
	26, 2, 1, 27, 2, 4, 5, 2, 
	7, 8, 2, 7, 9, 2, 7, 10, 
	2, 7, 11, 2, 7, 13, 2, 7, 
	14, 3, 7, 0, 11, 3, 7, 0, 
	13, 3, 7, 0, 14, 3, 7, 0, 
	15, 3, 7, 0, 16, 3, 7, 0, 
	17, 3, 7, 0, 18, 3, 7, 1, 
	11, 3, 7, 1, 12, 4, 7, 0, 
	1, 11, 4, 7, 0, 1, 12, 4, 
	7, 0, 2, 14, 4, 7, 2, 0, 
	14, 4, 7, 3, 0, 14, 5, 7, 
	1, 2, 0, 14
};

static const short _kegogi_lexer_key_offsets[] = {
	0, 0, 5, 7, 10, 17, 24, 26, 
	26, 30, 35, 40, 46, 47, 50, 54, 
	61, 68, 70, 70, 74, 79, 84, 87, 
	88, 93, 115, 118, 123, 128, 131, 135, 
	140, 143, 150, 162, 174, 187, 200, 213, 
	226, 239, 252, 265, 278, 291, 304, 317, 
	330, 343, 356
};

static const char _kegogi_lexer_trans_keys[] = {
	32, 47, 58, 9, 13, 48, 57, 47, 
	48, 57, 32, 34, 47, 58, 92, 9, 
	13, 32, 34, 47, 58, 92, 9, 13, 
	34, 92, 34, 92, 48, 57, 34, 47, 
	92, 48, 57, 32, 47, 58, 9, 13, 
	10, 32, 47, 58, 9, 13, 10, 10, 
	48, 57, 10, 47, 48, 57, 32, 41, 
	47, 58, 92, 9, 13, 32, 41, 47, 
	58, 92, 9, 13, 41, 92, 41, 92, 
	48, 57, 41, 47, 92, 48, 57, 32, 
	47, 58, 9, 13, 47, 48, 57, 47, 
	32, 47, 58, 9, 13, 10, 32, 34, 
	35, 40, 44, 47, 58, 61, 100, 101, 
	115, 123, 125, 9, 13, 48, 57, 65, 
	90, 97, 122, 32, 9, 13, 32, 47, 
	58, 9, 13, 32, 34, 92, 9, 13, 
	32, 9, 13, 10, 32, 9, 13, 32, 
	41, 92, 9, 13, 32, 9, 13, 32, 
	47, 58, 9, 13, 48, 57, 32, 45, 
	47, 58, 9, 13, 48, 57, 65, 90, 
	97, 122, 32, 45, 47, 58, 9, 13, 
	48, 57, 65, 90, 97, 122, 32, 45, 
	47, 58, 101, 9, 13, 48, 57, 65, 
	90, 97, 122, 32, 45, 47, 58, 102, 
	9, 13, 48, 57, 65, 90, 97, 122, 
	32, 45, 47, 58, 97, 9, 13, 48, 
	57, 65, 90, 98, 122, 32, 45, 47, 
	58, 117, 9, 13, 48, 57, 65, 90, 
	97, 122, 32, 45, 47, 58, 108, 9, 
	13, 48, 57, 65, 90, 97, 122, 32, 
	45, 47, 58, 116, 9, 13, 48, 57, 
	65, 90, 97, 122, 32, 45, 47, 58, 
	115, 9, 13, 48, 57, 65, 90, 97, 
	122, 32, 45, 47, 58, 120, 9, 13, 
	48, 57, 65, 90, 97, 122, 32, 45, 
	47, 58, 112, 9, 13, 48, 57, 65, 
	90, 97, 122, 32, 45, 47, 58, 101, 
	9, 13, 48, 57, 65, 90, 97, 122, 
	32, 45, 47, 58, 99, 9, 13, 48, 
	57, 65, 90, 97, 122, 32, 45, 47, 
	58, 116, 9, 13, 48, 57, 65, 90, 
	97, 122, 32, 45, 47, 58, 101, 9, 
	13, 48, 57, 65, 90, 97, 122, 32, 
	45, 47, 58, 110, 9, 13, 48, 57, 
	65, 90, 97, 122, 32, 45, 47, 58, 
	100, 9, 13, 48, 57, 65, 90, 97, 
	122, 0
};

static const char _kegogi_lexer_single_lengths[] = {
	0, 3, 0, 1, 5, 5, 2, 0, 
	2, 3, 3, 4, 1, 1, 2, 5, 
	5, 2, 0, 2, 3, 3, 1, 1, 
	3, 14, 1, 3, 3, 1, 2, 3, 
	1, 3, 4, 4, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5
};

static const char _kegogi_lexer_range_lengths[] = {
	0, 1, 1, 1, 1, 1, 0, 0, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	1, 0, 0, 1, 1, 1, 1, 0, 
	1, 4, 1, 1, 1, 1, 1, 1, 
	1, 2, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4
};

static const short _kegogi_lexer_index_offsets[] = {
	0, 0, 5, 7, 10, 17, 24, 27, 
	28, 32, 37, 42, 48, 50, 53, 57, 
	64, 71, 74, 75, 79, 84, 89, 92, 
	94, 99, 118, 121, 126, 131, 134, 138, 
	143, 146, 152, 161, 170, 180, 190, 200, 
	210, 220, 230, 240, 250, 260, 270, 280, 
	290, 300, 310
};

static const char _kegogi_lexer_indicies[] = {
	0, 2, 3, 0, 1, 4, 0, 5, 
	6, 0, 8, 9, 10, 11, 12, 8, 
	7, 14, 15, 16, 17, 18, 14, 13, 
	19, 20, 14, 14, 19, 20, 21, 14, 
	19, 22, 20, 23, 14, 14, 16, 17, 
	14, 13, 26, 25, 27, 28, 25, 24, 
	26, 25, 26, 29, 25, 26, 30, 31, 
	25, 33, 34, 35, 36, 37, 33, 32, 
	39, 40, 41, 42, 43, 39, 38, 44, 
	45, 39, 39, 44, 45, 46, 39, 44, 
	47, 45, 48, 39, 39, 41, 42, 39, 
	38, 49, 4, 0, 50, 0, 0, 0, 
	0, 0, 51, 26, 52, 53, 54, 55, 
	56, 57, 59, 60, 62, 63, 64, 65, 
	66, 52, 58, 61, 61, 51, 67, 67, 
	68, 0, 2, 3, 0, 1, 71, 72, 
	73, 71, 70, 71, 71, 70, 76, 75, 
	75, 74, 78, 79, 80, 78, 77, 78, 
	78, 77, 81, 83, 85, 81, 84, 82, 
	67, 86, 83, 87, 67, 86, 88, 88, 
	82, 89, 86, 83, 85, 89, 86, 86, 
	86, 82, 89, 86, 83, 87, 90, 89, 
	86, 88, 88, 82, 89, 86, 83, 87, 
	91, 89, 86, 88, 88, 82, 89, 86, 
	83, 87, 92, 89, 86, 88, 88, 82, 
	89, 86, 83, 87, 93, 89, 86, 88, 
	88, 82, 89, 86, 83, 87, 94, 89, 
	86, 88, 88, 82, 89, 86, 83, 87, 
	95, 89, 86, 88, 88, 82, 89, 86, 
	83, 87, 96, 89, 86, 88, 88, 82, 
	89, 86, 83, 87, 97, 89, 86, 88, 
	88, 82, 89, 86, 83, 87, 98, 89, 
	86, 88, 88, 82, 89, 86, 83, 87, 
	99, 89, 86, 88, 88, 82, 89, 86, 
	83, 87, 100, 89, 86, 88, 88, 82, 
	89, 86, 83, 87, 101, 89, 86, 88, 
	88, 82, 89, 86, 83, 87, 102, 89, 
	86, 88, 88, 82, 89, 86, 83, 87, 
	103, 89, 86, 88, 88, 82, 89, 86, 
	83, 87, 104, 89, 86, 88, 88, 82, 
	0
};

static const char _kegogi_lexer_trans_targs[] = {
	25, 1, 26, 2, 3, 26, 3, 5, 
	6, 27, 28, 8, 10, 5, 6, 27, 
	28, 8, 10, 25, 7, 9, 28, 9, 
	11, 12, 25, 30, 13, 14, 30, 14, 
	16, 17, 27, 31, 19, 21, 16, 17, 
	27, 31, 19, 21, 25, 18, 20, 31, 
	20, 23, 24, 1, 25, 4, 11, 15, 
	27, 26, 33, 25, 27, 34, 36, 43, 
	48, 27, 27, 25, 26, 25, 28, 6, 
	26, 29, 30, 12, 25, 31, 17, 26, 
	32, 25, 1, 26, 33, 2, 35, 22, 
	34, 25, 37, 38, 39, 40, 41, 42, 
	34, 44, 45, 46, 47, 34, 49, 50, 
	34
};

static const char _kegogi_lexer_trans_actions[] = {
	15, 0, 116, 5, 1, 121, 0, 1, 
	1, 101, 111, 17, 1, 0, 0, 93, 
	116, 5, 0, 23, 0, 1, 121, 0, 
	0, 0, 9, 116, 5, 1, 121, 0, 
	1, 1, 106, 111, 17, 1, 0, 0, 
	97, 116, 5, 0, 26, 0, 1, 121, 
	0, 0, 0, 1, 13, 1, 1, 1, 
	89, 73, 69, 11, 85, 65, 65, 65, 
	65, 77, 81, 41, 62, 38, 62, 3, 
	93, 62, 62, 3, 29, 62, 3, 97, 
	62, 35, 3, 126, 59, 20, 56, 20, 
	56, 32, 56, 56, 56, 56, 56, 56, 
	47, 56, 56, 56, 56, 53, 56, 56, 
	50
};

static const char _kegogi_lexer_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 44, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0
};

static const char _kegogi_lexer_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 7, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0
};

static const short _kegogi_lexer_eof_trans[] = {
	0, 1, 1, 1, 0, 0, 1, 1, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 1, 1, 0, 0, 0, 1, 1, 
	1, 0, 68, 1, 70, 70, 70, 70, 
	70, 82, 68, 90, 90, 90, 90, 90, 
	90, 90, 90, 90, 90, 90, 90, 90, 
	90, 90, 90
};

static const int kegogi_lexer_start = 25;
static const int kegogi_lexer_first_final = 25;
static const int kegogi_lexer_error = 0;

static const int kegogi_lexer_en_main = 25;


#line 88 "src/kegogi_lexer.rl"

TokenList *get_kegogi_tokens(bstring content) {
    TokenList *token_list = TokenList_create(1024);
    check_mem(token_list);

    char *p = bdata(content);
    char *pe = p + blength(content);
    char *m = NULL;
    char *eof = pe;
    int cs = -1;
    int act = -1;
    char *ts = NULL;
    char *te = NULL;

    bstring s1 = NULL;
    bstring s2 = NULL;
    bstring s3 = NULL;

    
#line 263 "src/kegogi_lexer.c"
	{
	cs = kegogi_lexer_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 107 "src/kegogi_lexer.rl"
    
#line 273 "src/kegogi_lexer.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_acts = _kegogi_lexer_actions + _kegogi_lexer_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 6:
#line 1 "src/kegogi_lexer.rl"
	{ts = p;}
	break;
#line 294 "src/kegogi_lexer.c"
		}
	}

	_keys = _kegogi_lexer_trans_keys + _kegogi_lexer_key_offsets[cs];
	_trans = _kegogi_lexer_index_offsets[cs];

	_klen = _kegogi_lexer_single_lengths[cs];
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

	_klen = _kegogi_lexer_range_lengths[cs];
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
	_trans = _kegogi_lexer_indicies[_trans];
_eof_trans:
	cs = _kegogi_lexer_trans_targs[_trans];

	if ( _kegogi_lexer_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _kegogi_lexer_actions + _kegogi_lexer_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 17 "src/kegogi_lexer.rl"
	{ m = p; }
	break;
	case 1:
#line 18 "src/kegogi_lexer.rl"
	{ s1 = blk2bstr(m, p-m); }
	break;
	case 2:
#line 19 "src/kegogi_lexer.rl"
	{ s2 = blk2bstr(m, p-m); }
	break;
	case 3:
#line 20 "src/kegogi_lexer.rl"
	{ s3 = blk2bstr(m, p-m); }
	break;
	case 7:
#line 1 "src/kegogi_lexer.rl"
	{te = p+1;}
	break;
	case 8:
#line 38 "src/kegogi_lexer.rl"
	{act = 1;}
	break;
	case 9:
#line 42 "src/kegogi_lexer.rl"
	{act = 2;}
	break;
	case 10:
#line 46 "src/kegogi_lexer.rl"
	{act = 3;}
	break;
	case 11:
#line 50 "src/kegogi_lexer.rl"
	{act = 4;}
	break;
	case 12:
#line 54 "src/kegogi_lexer.rl"
	{act = 5;}
	break;
	case 13:
#line 58 "src/kegogi_lexer.rl"
	{act = 6;}
	break;
	case 14:
#line 62 "src/kegogi_lexer.rl"
	{act = 7;}
	break;
	case 15:
#line 70 "src/kegogi_lexer.rl"
	{act = 9;}
	break;
	case 16:
#line 74 "src/kegogi_lexer.rl"
	{act = 10;}
	break;
	case 17:
#line 78 "src/kegogi_lexer.rl"
	{act = 11;}
	break;
	case 18:
#line 83 "src/kegogi_lexer.rl"
	{act = 13;}
	break;
	case 19:
#line 50 "src/kegogi_lexer.rl"
	{te = p+1;{
             TokenList_append1(&token_list, TKSTRING, s1);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 20:
#line 54 "src/kegogi_lexer.rl"
	{te = p+1;{
             TokenList_append1(&token_list, TKPATTERN, s1);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 21:
#line 66 "src/kegogi_lexer.rl"
	{te = p+1;{
             TokenList_append0(&token_list, TKNEWLINE);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 22:
#line 78 "src/kegogi_lexer.rl"
	{te = p+1;{
             TokenList_append0(&token_list, TKEQUALS);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 23:
#line 82 "src/kegogi_lexer.rl"
	{te = p+1;{ }}
	break;
	case 24:
#line 50 "src/kegogi_lexer.rl"
	{te = p;p--;{
             TokenList_append1(&token_list, TKSTRING, s1);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 25:
#line 58 "src/kegogi_lexer.rl"
	{te = p;p--;{
             TokenList_append1(&token_list, TKNUMBER, s1);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 26:
#line 62 "src/kegogi_lexer.rl"
	{te = p;p--;{
             TokenList_append3(&token_list, TKURL, s1, s2, s3);
             s1 = s2 = s3 = NULL;
         }}
	break;
	case 27:
#line 1 "src/kegogi_lexer.rl"
	{	switch( act ) {
	case 0:
	{{cs = 0; goto _again;}}
	break;
	case 1:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKDEFAULTS);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 2:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKSEND);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 3:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKEXPECT);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 4:
	{{p = ((te))-1;}
             TokenList_append1(&token_list, TKSTRING, s1);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 5:
	{{p = ((te))-1;}
             TokenList_append1(&token_list, TKPATTERN, s1);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 6:
	{{p = ((te))-1;}
             TokenList_append1(&token_list, TKNUMBER, s1);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 7:
	{{p = ((te))-1;}
             TokenList_append3(&token_list, TKURL, s1, s2, s3);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 9:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKDICT_START);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 10:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKDICT_END);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 11:
	{{p = ((te))-1;}
             TokenList_append0(&token_list, TKEQUALS);
             s1 = s2 = s3 = NULL;
         }
	break;
	case 13:
	{{p = ((te))-1;} }
	break;
	}
	}
	break;
#line 549 "src/kegogi_lexer.c"
		}
	}

_again:
	_acts = _kegogi_lexer_actions + _kegogi_lexer_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 4:
#line 1 "src/kegogi_lexer.rl"
	{ts = 0;}
	break;
	case 5:
#line 1 "src/kegogi_lexer.rl"
	{act = 0;}
	break;
#line 566 "src/kegogi_lexer.c"
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _kegogi_lexer_eof_trans[cs] > 0 ) {
		_trans = _kegogi_lexer_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

#line 108 "src/kegogi_lexer.rl"

    return token_list;

error:
    TokenList_destroy(token_list);
    return NULL;
}