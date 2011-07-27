%include {

#include <stdlib.h>
#include <stdio.h>
#include <dbg.h>
#include "ast.h"
#include <task/task.h>


}

%token_prefix TK

%token_type {Token*}
%extra_argument {ParserState *state}

%syntax_error {
    if( !TOKEN ) {
      log_err("Reached the end of file so it seems like you are missing a ')'");
    }
    state->error = 1;
}

%stack_overflow {
    log_err("There was a stack overflow at line: %d", state->line_number);
}

%token_destructor { Token_destroy($$); }

config ::= vars(V).  { state->settings = V; } 

%type vars { tst_t * }
vars(V) ::= vars(O) assignment(A). 
    { 
        V = tst_insert(O, bdata(A->key->data), blength(A->key->data), A);
    }

vars(V) ::= assignment(A). 
    {
        V = tst_insert(V, bdata(A->key->data), blength(A->key->data), A);
    }

vars(V) ::= vars(A) EOF. { V = A; }


%type expr { Value * }
expr(E) ::= QSTRING(A). { E = Value_create(VAL_QSTRING, A); }
expr(E) ::= NUMBER(A). { E = Value_create(VAL_NUMBER, A); }
expr(E) ::= class(A). { E = Value_create(VAL_CLASS, A); }
expr(E) ::= list(A). { E = Value_create(VAL_LIST, A); }
expr(E) ::= hash(A). { E = Value_create(VAL_HASH, A); }
expr(E) ::= IDENT(A). { E = Value_create(VAL_REF, A); }


%type assignment { Pair * }
%destructor assignment { free($$); }
assignment(A) ::= IDENT(I) EQ expr(E).  { 
        A = malloc(sizeof(Pair)); A->key = I; A->value = E; 
    }


%type class { Class *}
class(C) ::= CLASS(I) LPAREN parameters(P) RPAREN. 
      { C = calloc(sizeof(Class), 1); C->id = -1; C->ident = I; C->params = P; }

%type parameters { tst_t *}
%destructor parameters { AST_destroy($$); }
parameters(P) ::= parameters(O) COMMA assignment(A). 
    { P = tst_insert(O, bdata(A->key->data), blength(A->key->data), A); }

parameters(P) ::= parameters(O) assignment(A). 
    { P = tst_insert(O, bdata(A->key->data), blength(A->key->data), A); }

parameters(P) ::= .  
    { P = NULL; }


%type list { list_t *}
list(L) ::= LBRACE list_elements(E) RBRACE.  { L = E; }

%type list_elements { list_t * }
list_elements(L) ::= list_elements(M) COMMA expr(E).
    { L = M; list_append(L, lnode_create(E)); }

list_elements(L) ::= list_elements(M) expr(E).
    { L = M; list_append(L, lnode_create(E)); }

list_elements(L) ::= .
    { L = list_create(LISTCOUNT_T_MAX); }


%type hash { tst_t *}
hash(H) ::= LBRACKET hash_elements(E) RBRACKET.  { H = E; }

%type hash_elements { tst_t * }
hash_elements(H) ::= hash_elements(E) COMMA hash_pair(P).
    { H = tst_insert(E, bdata(P->key->data), blength(P->key->data), P); }

hash_elements(H) ::= hash_elements(E) hash_pair(P).
    { H = tst_insert(E, bdata(P->key->data), blength(P->key->data), P); }

hash_elements(H) ::= . 
    { H = NULL; }


%type hash_pair { Pair* }
%destructor hash_pair { free($$); }
hash_pair(P) ::= QSTRING(A) COLON expr(B).  { 
        P = malloc(sizeof(Pair)); P->key = A; P->value = B; 
    }

