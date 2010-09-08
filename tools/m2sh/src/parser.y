%include {

#include <stdlib.h>
#include <stdio.h>
#include <dbg.h>
#include "ast.h"
#include <task/task.h>

#define assert(S) if(!(S)) { log_err("PARSER ASSERT FAILED: " #S); taskexitall(-1); }

}

%token_prefix TK

%token_type {Token*}
%extra_argument {ParserState *state}

%syntax_error {
    state->error = 1;
}

%stack_overflow {
    log_err("There was a stack overflow at line: %d", state->line_number);
}

%token_destructor { Token_destroy($$); }

config ::= vars(V).  { state->settings = V; } 

%type vars { hash_t * }
vars(V) ::= vars(O) assignment(A). 
    { V = O;  hash_alloc_insert(V, bdata(A.key->data), A.value); }

vars(V) ::= assignment(A). 
    {
    V = hash_create(HASHCOUNT_T_MAX, NULL, NULL);
    hash_alloc_insert(V, bdata(A.key->data), A.value); 
    }

vars(V) ::= vars(A) EOF. { V = A; }


%type expr { Value * }
expr(E) ::= QSTRING(A). { E = Value_create(VAL_QSTRING, A); }
expr(E) ::= PATTERN(A). { E = Value_create(VAL_PATTERN, A); }
expr(E) ::= NUMBER(A). { E = Value_create(VAL_NUMBER, A); }
expr(E) ::= class(A). { E = Value_create(VAL_CLASS, A); }
expr(E) ::= list(A). { E = Value_create(VAL_LIST, A); }
expr(E) ::= hash(A). { E = Value_create(VAL_HASH, A); }
expr(E) ::= IDENT(A). { E = Value_create(VAL_REF, A); }


%type assignment { Pair }
assignment(A) ::= IDENT(I) EQ expr(E).  { A.key = I; A.value = E; }


%type class { Class *}
class(C) ::= CLASS(I) LPAREN parameters(P) RPAREN. 
      {
          debug("CLASS: %s with %d", bdata(I->data), (int)hash_count(P)); 
          C = calloc(sizeof(Class), 1); C->ident = I; C->params = P;
      }

%type parameters { hash_t *}
parameters(P) ::= parameters(O) COMMA assignment(A). 
    { P = O; hash_alloc_insert(P, bdata(A.key->data), A.value); }

parameters(P) ::= parameters(O) assignment(A). 
    { P = O; hash_alloc_insert(P, bdata(A.key->data), A.value); }

parameters(P) ::= .  
    { P = hash_create(HASHCOUNT_T_MAX, NULL, NULL); }


%type list { list_t *}
list(L) ::= LBRACE list_elements(E) RBRACE.
    { L = E; debug("LIST: %d", (int)list_count(L)); }


%type list_elements { list_t * }
list_elements(L) ::= list_elements(M) COMMA expr(E).
    { L = M; list_append(L, lnode_create(E)); }

list_elements(L) ::= list_elements(M) expr(E).
    { L = M; list_append(L, lnode_create(E)); }

list_elements(L) ::= .
    { L = list_create(LISTCOUNT_T_MAX); }


%type hash { hash_t *}
hash(H) ::= LBRACKET hash_elements(E) RBRACKET.  
    { H = E; debug("HASH: %d", (int)hash_count(H)); }

%type hash_elements { hash_t * }
hash_elements(H) ::= hash_elements(E) COMMA hash_pair(P).
    { H = E; hash_alloc_insert(H, bdata(P.key->data), P.value); }

hash_elements(H) ::= hash_elements(E) hash_pair(P).
    { H = E; hash_alloc_insert(H, bdata(P.key->data), P.value); }

hash_elements(H) ::= . 
    { H = hash_create(HASHCOUNT_T_MAX, NULL, NULL); }


%type hash_pair { Pair }
hash_pair(P) ::= QSTRING(A) COLON expr(B).  { P.key = A; P.value = B; }
hash_pair(P) ::= PATTERN(A) COLON expr(B).  { P.key = A; P.value = B; }


