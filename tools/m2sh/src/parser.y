%include {

#include <stdlib.h>
#include <stdio.h>
#include <dbg.h>
#include "token.h"
#include "parser.h"
#include <task/task.h>
#include <adt/list.h>
#include <adt/hash.h>

#define assert(S) {                                     \
        if(!(S)) {                                      \
            debug(#S);                                  \
            taskexitall(-1);                            \
        }                                               \
    }

typedef struct Pair {
    Token *key;
    void *value;
} Pair;

typedef struct Class {
    Token *ident;
    hash_t *params;
} Class;

typedef enum ValueType {
    VAL_QSTRING, VAL_PATTERN, VAL_NUMBER, VAL_CLASS, VAL_LIST, VAL_HASH, VAL_IDENT
} ValueType;

typedef struct Value {
    ValueType type;    
    void *data;
} Value;

static inline Value *Value_create(ValueType type, void *data) {
    Value *val = calloc(sizeof(Value), 1);
    val->type = type; val->data = data;
    return val;
}

}

%token_prefix TK

%token_type {Token*}
%extra_argument {hash_t **settings}

%syntax_error {
    log_err("There was a syntax error");
}

%stack_overflow {
    log_err("There was a stack overflow");
}


config ::= vars(V).  { *settings = V; } 

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
expr(E) ::= IDENT(A). { E = Value_create(VAL_IDENT, A); }


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


