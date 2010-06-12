/*
 * SFX---A utility which tries to determine whether a given C expression
 * is free of side effects. This can be used for verifying that macros which
 * expand their arguments more than once are not being accidentally misused.
 *
 * Copyright (C) 1999 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: sfx.c,v 1.30 1999/11/13 08:41:55 kaz Exp $
 * $Name: kazlib_1_20 $
 */

#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "except.h"
#include "sfx.h"
#include "hash.h"
#ifdef KAZLIB_POSIX_THREADS
#include <pthread.h>
#endif

#ifdef KAZLIB_RCSID
static const char rcsid[] = "$Id: sfx.c,v 1.30 1999/11/13 08:41:55 kaz Exp $";
#endif

/*
 * Exceptions
 */

#define SFX_EX		0x34DB9C4A
#define SFX_SYNERR	1

/*
 * Cache entry
 */

typedef struct {
    hnode_t node;
    const char *expr;
    sfx_rating_t eff;
} sfx_entry_t;

/*
 * Parsing context structure
 */

typedef struct {
    const unsigned char *start;
    const unsigned char *input;
    size_t size;
    sfx_rating_t eff;
} context_t;

/*
 * Declarator type: abstract, concrete or both
 */

typedef enum {
    decl_abstract, decl_concrete, decl_both
} decl_t;

static void init_context(context_t *ctx, const unsigned char *expr)
{
    ctx->input = ctx->start = expr;
    ctx->size = strlen((const char *) expr) + 1;
    ctx->eff = sfx_none;
}

static void assign_context(context_t *copy, context_t *orig)
{
    *copy = *orig;
}

static void set_effect(context_t *ctx, sfx_rating_t eff)
{
    assert (eff == sfx_none || eff == sfx_potential || eff == sfx_certain);

    if (eff > ctx->eff)
	ctx->eff = eff;
}

static void reset_effect(context_t *ctx)
{
    ctx->eff = sfx_none;
}

static sfx_rating_t get_effect(context_t *ctx)
{
    return ctx->eff;
}

static int skip_ws(context_t *expr)
{
    while (*expr->input != 0 && isspace(*expr->input))
	expr->input++;

    return (*expr->input == 0);
}

static int get_next(context_t *expr)
{
    int ret = *expr->input;
    if (ret)
	expr->input++;
    return ret;
}

static int get_next_skip_ws(context_t *expr)
{
    if (!skip_ws(expr))
	return *expr->input++;
    return 0;
}

static const unsigned char *get_ptr(context_t *expr)
{
    return expr->input;
}

static void skip_n(context_t *ctx, size_t n)
{
    assert ((size_t) (ctx->input - ctx->start) <= ctx->size - n);
    ctx->input += n;
}

static void put_back(context_t *expr, int ch)
{
    if (ch)
	expr->input--;
}

static int peek_next(context_t *expr)
{
    return *expr->input;
}

static void syntax_error(void)
{
    except_throw(SFX_EX, SFX_SYNERR, "syntax_error");
}

static void match_hard(context_t *expr, int match)
{
    int ch = get_next(expr);
    if (ch != match)
	syntax_error();
}

static void chk_comma(context_t *);

static void skip_ident(context_t *expr)
{
    int ch = get_next(expr);

    if (!isalpha(ch) && ch != '_')
	syntax_error();

    do {
	ch = get_next(expr);
    } while (isalnum(ch) || ch == '_');

    put_back(expr, ch);
}

static void skip_constant(context_t *expr)
{
    int ch = get_next(expr);

    assert (isdigit(ch) || ch == '.');

    do {
	ch = get_next(expr);
	if (ch == 'e' || ch == 'E') {
	    ch = get_next(expr);
	    if (ch == '+' || ch == '-') {
		ch = get_next(expr);
		if (!isdigit(ch))
		    syntax_error();
	    }
	}
    } while (ch != 0 && (isalnum(ch) || ch == '.'));

    put_back(expr, ch);
}

static void skip_strlit(context_t *expr)
{
    int ch = get_next(expr);

    assert (ch == '"');

    do {
	ch = get_next(expr);
	if (ch == '\\') {
	    get_next(expr);
	    continue;
	}
    } while (ch != 0 && ch != '"');

    if (ch != '"')
	syntax_error();
}

static void skip_charlit(context_t *expr)
{
    int ch = get_next(expr);

    assert (ch == '\'');

    do {
	ch = get_next(expr);
	if (ch == '\\') {
	    get_next(expr);
	    continue;
	}
    } while (ch != 0 && ch != '\'');

    if (ch != '\'')
	syntax_error();
}

static void chk_spec_qual_list(context_t *expr)
{
    skip_ws(expr);
    skip_ident(expr);

    for (;;) {
	int ch;

	skip_ws(expr);
	ch = peek_next(expr);

	if (!isalpha(ch) && ch != '_')
	    break;

	skip_ident(expr);
    }
}

static int speculate(void (*chk_func)(context_t *), context_t *expr, context_t *copy, int nextchar)
{
    static const except_id_t catch[] = { { SFX_EX, XCEPT_CODE_ANY } };
    except_t *ex;
    volatile int result = 0;
    assign_context(copy, expr);

    except_try_push(catch, 1, &ex);

    if (ex == 0) {
	chk_func(copy);
	if (nextchar) {
	    skip_ws(copy);
	    match_hard(copy, nextchar);
	}
	result = 1;
    } 

    except_try_pop();

    return result;
}

static void chk_pointer_opt(context_t *expr)
{
    for (;;) {
	int ch = get_next_skip_ws(expr);

	if (ch != '*') {
	    put_back(expr, ch);
	    break;
	}

	skip_ws(expr);

	ch = peek_next(expr);

	if (ch == '*')
	    continue;
	if (!isalpha(ch) && ch != '_')
	    break;

	skip_ident(expr);
    }
}

static void chk_decl(context_t *, decl_t);

static void chk_parm_decl(context_t *expr)
{
    chk_spec_qual_list(expr);
    chk_decl(expr, decl_both);
}

static void chk_parm_type_list(context_t *expr)
{
    for (;;) {
	int ch;

	chk_parm_decl(expr);

	ch = get_next_skip_ws(expr);

	if (ch != ',') {
	    put_back(expr, ch);
	    break;
	}

	ch = get_next_skip_ws(expr);

	if (ch == '.') {
	    match_hard(expr, '.');
	    match_hard(expr, '.');
	    break;
	}

	put_back(expr, ch);
    }
}

static void chk_conditional(context_t *);

static void chk_direct_decl(context_t *expr, decl_t type)
{
    for (;;) {
	int ch = get_next_skip_ws(expr);

	if (ch == '(') {
	    skip_ws(expr);
	    ch = peek_next(expr);
	    if (ch == '*' || ch == '(' || ch == '[')
		chk_decl(expr, type);
	    else if (isalpha(ch) || ch == '_')
		chk_parm_type_list(expr);
	    match_hard(expr, ')');
	} else if (ch == '[') {
	    skip_ws(expr);
	    ch = peek_next(expr);
	    if (ch != ']')
		chk_conditional(expr);
	    match_hard(expr, ']');		
	} else if ((type == decl_concrete || type == decl_both) && (isalpha(ch) || ch == '_')) {
	    put_back(expr, ch);
	    skip_ident(expr);
	    break;
	} else {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_decl(context_t *expr, decl_t type)
{
    int ch;
    chk_pointer_opt(expr);
    skip_ws(expr);
    ch = peek_next(expr);
    if (ch == '[' || ch == '(' || ((type == decl_concrete || type == decl_both) && (isalpha(ch) || ch == '_'))) {
	chk_direct_decl(expr, type);
    } 
}

static void chk_typename(context_t *expr)
{
    chk_spec_qual_list(expr);
    chk_decl(expr, decl_abstract);
}

static void chk_primary(context_t *expr)
{
    int ch = peek_next(expr);

    if (ch == 'L') {
	get_next(expr);
	ch = peek_next(expr);

	if (ch == '\'') {
	    skip_charlit(expr);
	    return;
	}

	if (ch == '"') {
	    skip_strlit(expr);
	    return;
	}

	put_back(expr, 'L');
	ch = 'L';
    }

    if (isalpha(ch) || ch == '_') {
    	skip_ident(expr);
	return;
    }

    if (isdigit(ch) || ch == '.') {
	skip_constant(expr);
	return;
    }

    if (ch == '(') {
	get_next(expr);
	chk_comma(expr);
	match_hard(expr, ')');
	return;
    }

    if (ch == '\'') {
	skip_charlit(expr);
	return;
    }

    if (ch == '"') {
	skip_strlit(expr);
	return;
    }

    syntax_error();
}

static void chk_postfix(context_t *expr)
{
    chk_primary(expr);

    for (;;) {
	int ch = get_next_skip_ws(expr);

	switch (ch) {
	case '[':
	    chk_comma(expr);
	    skip_ws(expr);
	    match_hard(expr, ']');
	    continue;
	case '(':
	    set_effect(expr, sfx_potential);
	    ch = get_next_skip_ws(expr);

	    if (ch != ')') {
		put_back(expr, ch);
		/* clever hack: parse non-empty argument list as comma expression */
		chk_comma(expr);
		ch = get_next_skip_ws(expr);
	    }

	    if (ch != ')')
		syntax_error();

	    continue;
	case '.':
	    skip_ws(expr);
	    skip_ident(expr);
	    continue;
	case '-':
	    ch = get_next(expr);

	    if (ch != '-' && ch != '>') {
		put_back(expr, ch);
		put_back(expr, '-');
		break;
	    }

	    if (ch == '>') {
		skip_ws(expr);
		skip_ident(expr);
		continue;
	    }

	    set_effect(expr, sfx_certain);
	    continue;
	case '+':
	    ch = get_next(expr);
	    if (ch != '+') {
		put_back(expr, ch);
		put_back(expr, '+');
		break;
	    }

	    set_effect(expr, sfx_certain);
	    continue;
	default:
	    put_back(expr, ch);
	    break;
	}
	break;
    }
}

static void chk_cast(context_t *);

static void chk_unary(context_t *expr)
{
    for (;;) {
	int nscan, ch = get_next_skip_ws(expr);

	switch (ch) {
	case '+':
	    ch = get_next(expr);
	    if (ch == '+')
		set_effect(expr, sfx_certain);
	    else
		put_back(expr, ch);
	    chk_cast(expr);
	    break;
	case '-':
	    ch = get_next(expr);
	    if (ch == '-')
		set_effect(expr, sfx_certain);
	    else
		put_back(expr, ch);
	    chk_cast(expr);
	    break;
	case '&': case '*': case '~': case '!':
	    chk_cast(expr);
	    break;
	case 's':
	    put_back(expr, ch);
	    nscan = 0;
	    sscanf((const char *) get_ptr(expr), "sizeof%*1[^a-z0-9_]%n", &nscan);

	    if (nscan == 7 || strcmp((const char *) get_ptr(expr), "sizeof") == 0) {
		sfx_rating_t eff = get_effect(expr);

	    	skip_n(expr, 6);

		ch = get_next_skip_ws(expr);

		if (ch == '(') {
		    context_t comma, type;
		    int iscomma = speculate(chk_comma, expr, &comma, ')');
		    int istype = speculate(chk_typename, expr, &type, ')');

		    if (!iscomma && !istype)
			syntax_error();

		    if (iscomma) {
			context_t unary;
			put_back(expr, ch);
			if (speculate(chk_unary, expr, &unary, 0)) {
			    assign_context(expr, &unary);
			    istype = 0;
			}
		    }

		    if (istype)
			assign_context(expr, &type);
		} else {
		    put_back(expr, ch);
		    chk_unary(expr);
		}

		reset_effect(expr);
		set_effect(expr, eff);
		break;
	    }
	    chk_postfix(expr);
	    break;
	default:
	    put_back(expr, ch);
	    chk_postfix(expr);
	    break;
	}

	break;
    }
}

static void chk_cast(context_t *expr)
{
    enum {
	parexpr,	/* parenthesized expression */
	partype,	/* parenthesized type name */
	parambig,	/* ambiguity between paren expr and paren type name */
	unary,		/* unary expression */
	plunary,	/* unary expression with leading plus or minus */
	other		/* none of the above, or even end of input */
    } curr = partype, old = partype, peek = partype;

    /* history for backtracking: two cast expression elements back */
    context_t old_expr = { 0 }, cur_expr = { 0 };

    for (;;) {
	context_t type, comma, unr;
	int ch = get_next_skip_ws(expr);

	/*
	 * Determine what the next bit of input is: parenthesized type name,
	 * expression, unary expression or what?  Speculative parsing is used
	 * to test several hypotheses.  For example,  something like
	 * (X)(Y) ^ 1 is seen, it will be turned, by subsequent iterations of
	 * this loop, into the codes: parambig, parambig, other.
	 */

	if (ch == '(') {
	    int istype = speculate(chk_typename, expr, &type, ')');
	    int iscomma = speculate(chk_comma, expr, &comma, ')');

	    switch (istype << 1 | iscomma) {
	    case 0:
		ch = get_next_skip_ws(expr);
		if (ch == ')')
		    peek = other; /* empty parentheses */
		else
		    syntax_error();
		break;
	    case 1:
		peek = parexpr;
		break;
	    case 2:
		peek = partype;
		break;
	    case 3:
		peek = parambig;
		break;
	    }
	    put_back(expr, ch);
	} else if (ch == 0) {
	    peek = other;
	} else {
	    put_back(expr, ch);
	    if (speculate(chk_unary, expr, &unr, 0)) {
		peek = (ch == '+' || ch == '-' || ch == '*' || ch == '&') ? plunary : unary;
	    } else {
		peek = other;
	    }
	}

	/*
	 * Okay, now we have an idea what is coming in the input. We make some
	 * sensible decision based on this and the thing we parsed previously.
	 * Either the parsing continues to grab more parenthesized things, or
	 * some decision is made to parse out the suffix material sensibly and
	 * terminate.  Backtracking is used up to two elements back. For
	 * example in the case of (X)(Y) ^ 1 (parambig, parambig, other) it's
	 * necessary, upon seeing ^ 1 (other) to go back to second to last
	 * ambigous parenthesized element (X) and terminate by parsing the
	 * (X)(Y) as a postfix expression. It cannot be a cast, because ^1
	 * isn't an expression.  Unary expressions that start with + or -
	 * create an interesting ambiguity.  Is (X)(Y) + 1 the addition of 1 to
	 * the result of the call to function X with parameter Y? Or is it the
	 * unary expression + 1 cast to type Y and X? The safer assumption is
	 * to go with the function call hypothesis, since that's the
	 * interpretation that may have side effects. 
	 */

	switch (curr) {
	case parexpr:		/* impossible cases */
	case other:
	case unary:
	case plunary:
	    assert (0);
	    syntax_error();
	    /* notreached */
	case partype:
	    switch (peek) {
	    case parexpr:	/* cast in front of parenthesized expression */
		chk_postfix(expr);
		return;
	    case partype:	/* compounding cast: keep looping */
		break;
	    case parambig:	/* type or expr: keep looping */
		break;
	    case unary:
	    case plunary:
		chk_unary(expr);
		return;
	    case other:		/* cast in front of non-expression! */
		syntax_error();
		/* notreached */
	    }
	    break;
	case parambig:
	    switch (peek) {
	    case parexpr:	/* function call */
		assign_context(expr, &cur_expr);
		chk_postfix(expr);
		return;
	    case partype:	/* compounding cast: keep looping */
		break;
	    case parambig:	/* type or expr: keep looping */
		break;
	    case unary:
		chk_unary(expr);
		return;
	    case plunary:	/* treat unary expr with + or - as additive */
	    case other:
		if (old == parambig) {
		    /* reparse two expression-like things in a row as call */
		    assign_context(expr, &old_expr);
		    chk_postfix(expr);
		    return;
		}
		/* reparse expression followed by non-parenthesized 
		   stuff as postfix expression */
		assign_context(expr, &cur_expr);
		chk_postfix(expr);
		return;		/* need more context */
	    }
	    break;
	}

	old = curr;
	curr = peek;
	assign_context(&old_expr, &cur_expr);
	assign_context(&cur_expr, expr);
	assign_context(expr, &type);
    }
}

static void chk_multiplicative(context_t *expr)
{
    for (;;) {
	int ch;

	chk_cast(expr);
	ch = get_next_skip_ws(expr);

	if ((ch != '*' && ch != '/' && ch != '%') || peek_next(expr) == '=') {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_additive(context_t *expr)
{
    for (;;) {
	int ch;

	chk_multiplicative(expr);
	ch = get_next_skip_ws(expr);

	if ((ch != '+' && ch != '-') || peek_next(expr) == '=') {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_shift(context_t *expr)
{
    for (;;) {
	int ch;

	chk_additive(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '<' && ch != '>') {
	    put_back(expr, ch);
	    break;
	}

	if (ch == '<' && peek_next(expr) != '<') {
	    put_back(expr, ch);
	    break;
	}

	if (ch == '>' && peek_next(expr) != '>') {
	    put_back(expr, ch);
	    break;
	}

	get_next(expr);

	if (peek_next(expr) == '=') {
	    put_back(expr, ch);
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_relational(context_t *expr)
{
    for (;;) {
	int ch;

	chk_shift(expr);
	ch = get_next_skip_ws(expr);

	
	if (ch != '<' && ch != '>') {
	    put_back(expr, ch);
	    break;
	}

	if (ch == '<' && peek_next(expr) == '<') {
	    put_back(expr, ch);
	    break;
	}

	if (ch == '>' && peek_next(expr) == '>') {
	    put_back(expr, ch);
	    break;
	}

	if (peek_next(expr) == '=')
	    get_next(expr);
    }
}

static void chk_equality(context_t *expr)
{
    for (;;) {
	int ch;

	chk_relational(expr);
	ch = get_next_skip_ws(expr);

	if ((ch != '!' && ch != '=') || peek_next(expr) != '=') {
	    put_back(expr, ch);
	    break;
	}

	match_hard(expr, '=');
    }
}

static void chk_and(context_t *expr)
{
    for (;;) {
	int ch;

	chk_equality(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '&' || peek_next(expr) == '&' || peek_next(expr) == '=') {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_exclusive_or(context_t *expr)
{
    for (;;) {
	int ch;

	chk_and(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '^' || peek_next(expr) == '=') {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_inclusive_or(context_t *expr)
{
    for (;;) {
	int ch;

	chk_exclusive_or(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '|' || peek_next(expr) == '|' || peek_next(expr) == '=') {
	    put_back(expr, ch);
	    break;
	}
    }
}

static void chk_logical_and(context_t *expr)
{
    for (;;) {
	int ch;

	chk_inclusive_or(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '&' || peek_next(expr) != '&') {
	    put_back(expr, ch);
	    break;
	}

	match_hard(expr, '&');
    }
}

static void chk_logical_or(context_t *expr)
{
    for (;;) {
	int ch;

	chk_logical_and(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '|' || peek_next(expr) != '|') {
	    put_back(expr, ch);
	    break;
	}

	match_hard(expr, '|');
    }
}

static void chk_conditional(context_t *expr)
{
    for (;;) {
	int ch;

	chk_logical_or(expr);
	ch = get_next_skip_ws(expr);

	if (ch != '?') {
	    put_back(expr, ch);
	    break;
	}

	chk_comma(expr);
	
	skip_ws(expr);
	match_hard(expr, ':');
    }
}

static void chk_assignment(context_t *expr)
{
    for (;;) {
	int ch;

	chk_conditional(expr);
	ch = get_next_skip_ws(expr);

	switch (ch) {
	case '=':
	    break;
	case '*': case '/': case '%':
	case '+': case '-': case '&':
	case '^': case '|':
	    match_hard(expr, '=');
	    break;
	case '<':
	    match_hard(expr, '<');
	    match_hard(expr, '=');
	    break;
	case '>':
	    match_hard(expr, '>');
	    match_hard(expr, '=');
	    break;
	case 0:
	default:
	    put_back(expr, ch);
	    return;
	}
	set_effect(expr, sfx_certain);
    }
}

static void chk_comma(context_t *expr)
{
    for (;;) {
	int ch;

	chk_assignment(expr);
	ch = get_next_skip_ws(expr);

	if (ch != ',') {
	    put_back(expr, ch);
	    break;
	}
    }
}

/*
 * This function returns 1 if the expression is successfully parsed,
 * or 0 if there is a syntax error.
 * 
 * The object pointed to by eff is set to indicate the side effect ranking of
 * the parsed expression: sfx_none, sfx_potential and sfx_certain.  These
 * rankins mean, respectively, that there are no side effects, that there are
 * potential side effects, or that there certainly are side effects.
 */

int sfx_determine(const char *expr, sfx_rating_t *eff)
{
    static const except_id_t catch[] = { { SFX_EX, XCEPT_CODE_ANY } };
    except_t *ex;
    context_t ctx;
    volatile int retval = 1;

    if (!except_init())
	return 0;

    init_context(&ctx, (const unsigned char *) expr);

    except_try_push(catch, 1, &ex);

    if (ex == 0) {
	chk_comma(&ctx);
	skip_ws(&ctx);
	if (peek_next(&ctx) != 0)
	    syntax_error();
    } else {
	/* exception caught */
	retval = 0;
    }

    except_try_pop();

    *eff = ctx.eff;

    except_deinit();

    return retval;
}


#ifdef KAZLIB_POSIX_THREADS

static pthread_once_t cache_init;
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

#define init_once(X, Y) pthread_once(X, Y)
#define lock_cache() pthread_mutex_lock(&cache_mutex)
#define unlock_cache() pthread_mutex_unlock(&cache_mutex)

#else
static int cache_init;

static void init_once(int *once, void (*func)(void))
{
    if (*once == 0) {
	func();
	*once = 1;
    }
}

#define lock_cache()
#define unlock_cache()
#endif

static hash_t *cache;

extern hash_t *hash_create(hashcount_t, hash_comp_t, hash_fun_t);

static void init_cache(void)
{
    cache = hash_create(HASHCOUNT_T_MAX, 0, 0);
}

static int lookup_cache(const char *expr, sfx_rating_t *rating)
{
    hnode_t *cache_node;
    init_once(&cache_init, init_cache);

    lock_cache();

    cache_node = hash_lookup(cache, expr);

    unlock_cache();

    if (cache_node != 0) {
	sfx_entry_t *cache_entry = hnode_get(cache_node);
	*rating = cache_entry->eff;
	return 1;
    }

    return 0;
}

static int cache_result(const char *expr, sfx_rating_t rating)
{
    int result = 0;
    hnode_t *cache_node;

    init_once(&cache_init, init_cache);

    if (cache == 0)
	goto bail;

    lock_cache();

    cache_node = hash_lookup(cache, expr);

    if (!cache_node) {
	sfx_entry_t *cache_entry = malloc(sizeof *cache_entry);

	if (cache_entry == 0)
	    goto bail_unlock;

	hnode_init(&cache_entry->node, cache_entry);
	cache_entry->expr = expr;
	cache_entry->eff = rating;
	hash_insert(cache, &cache_entry->node, expr);
    } else {
	sfx_entry_t *cache_entry = hnode_get(cache_node);
	cache_entry->eff = rating;
	result = 1;
    }

    result = 1;

    
bail_unlock:
    unlock_cache();

bail:
    return result;
}


void sfx_check(const char *expr, const char *file, unsigned long line)
{
    sfx_rating_t eff;
    int success = lookup_cache(expr, &eff);

    if (!success) {
	success = sfx_determine(expr, &eff);
	cache_result(expr, eff);
    }

    if (!success) {
	fprintf(stderr, "%s:%ld: syntax error in expression \"%s\"\n",
		file, line, expr);
    } else if (eff == sfx_potential) {
	fprintf(stderr, "%s:%ld: expression \"%s\" may have side effects\n",
		file, line, expr);
    } else if (eff == sfx_certain) {
	fprintf(stderr, "%s:%ld: expression \"%s\" has side effects\n",
		file, line, expr);
    } else {
	return;
    }
}

int sfx_declare(const char *expr, sfx_rating_t eff)
{
    return cache_result(expr, eff);
}

#ifdef KAZLIB_TEST_MAIN

#include <stdlib.h>

int main(int argc, char **argv)
{
    char expr_buf[256];
    char *expr, *ptr;
    sfx_rating_t eff;

    for (;;) {
	if (argc < 2) {
	    expr = expr_buf;
	    if (fgets(expr_buf, sizeof expr_buf, stdin) == 0)
		break;
	    if ((ptr = strchr(expr_buf, '\n')) != 0)
		*ptr = 0;
	} else {
	    expr = (argv++)[1];
	    if (!expr)
		break;
	}

	if (!sfx_determine(expr, &eff)) {
	    printf("expression '%s' has a syntax error\n", expr);
	    return EXIT_FAILURE;
	}

	switch (eff) {
	case sfx_none:
	    printf("expression '%s' has no side effects\n", expr);
	    break;
	case sfx_potential:
	    printf("expression '%s' may have side effects\n", expr);
	    break;
	case sfx_certain:
	    printf("expression '%s' has side effects\n", expr);
	    break;
	}
    }

    return 0;
}

#endif
