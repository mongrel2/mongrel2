%include {

#include <stdlib.h>
#include <stdio.h>
#include <dbg.h>
#include "kegogi_parser.h"
#include "kegogi_tokens.h"
#include "kegogi.h"

#define assert(S) {                                     \
        if(!(S)) {                                      \
            debug(#S);                                  \
            taskexitall(-1);                            \
        }                                               \
    }

}

%token_prefix TK

%extra_argument {CommandList *commandList}

%type param {Param*}
%type params {ParamDict*}
%type dict {ParamDict*}
%type defaults {ParamDict*}
%type command {Command*}

%destructor command { free($$); }
%token_type {Token*}

%syntax_error {
    debug("There was a syntax error");
 }

%stack_overflow {
    debug("There was a stack overflow");
 }
script ::= command_list.

command_list ::= command_list command(A) . {
    int idx = commandList->count;
    assert(idx < commandList->size);
    commandList->count++;
    commandList->commands[idx] = *A;
    free(A);
}

command_list ::= command_list defaults(A) . {
    if(commandList->defaults != NULL)
        debug("You can only specify defaults once");
    else
        commandList->defaults = A;
}

command_list ::= command_list NEWLINE .

command_list ::= .

params(A) ::= params(B) param(C) . { A = B; ParamDict_set(A, C); }
params(A) ::= . { A = ParamDict_create(); }

param(A) ::= STRING(B) EQUALS PATTERN(C) . {
    A = Param_create(bstrcpy(B->s1), PATTERN, bstrcpy(C->s1));
}    

param(A) ::= STRING(B) EQUALS STRING(C) . {
    A = Param_create(bstrcpy(B->s1), STRING, bstrcpy(C->s1));
}

param(A) ::= STRING(B) EQUALS DICT_START params(C) DICT_END . {
    A = Param_create(bstrcpy(B->s1), DICT, C);
}

defaults(A) ::= DEFAULTS params(B) NEWLINE . { A = B; }

command(A) ::= SEND STRING(B) URL(C) params(D) NEWLINE
               EXPECT NUMBER(E) params(F) NEWLINE . {
           A = calloc(sizeof(Command), 1);
           A->send.method = bstrcpy(B->s1);
           A->send.uri = bstrcpy(C->s1);
           A->send.host = bstrcpy(C->s2);
           A->send.port = bstrcpy(C->s3);
           A->send.params = D;
           A->expect.status_code = bstrcpy(E->s1);
           A->expect.params = F;
}

