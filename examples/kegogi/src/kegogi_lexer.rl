#include "kegogi_tokens.h"
#include "kegogi_parser.h"

#include <stdio.h>

#include <bstring.h>
#include <dbg.h>

%%{
    machine kegogi_lexer;
    defaults = 'defaults';
    send = 'send';
    expect = 'expect';

    comment = '#' [^\n]*;

    action mark { m = fpc; }
    action s1 { s1 = blk2bstr(m, fpc-m); }
    action s2 { s2 = blk2bstr(m, fpc-m); }
    action s3 { s3 = blk2bstr(m, fpc-m); }

    ustring = (alpha (alnum | '-')*) >mark %s1;
    number = (digit+) >mark %s1;
    qstring = '\"' (([^\\\"] | ('\\' any))*) >mark %s1 '\"';
    pattern = '(' (([^\\)] | ('\\' any))*) >mark %s1 ')';
    dict_start = '{';
    dict_end = '}';
    equals = [:=];

    host = [^/: ]+ >mark %s2;
    uri = ('/' (any - space)*) >mark %s1;
    port = digit+ >mark %s3;
    url = ((alpha+ '://')? host (':' port)?)? uri;
    
    newline = '\n';

    main := |*
         defaults => {
             TokenList_append0(&token_list, TKDEFAULTS);
             s1 = s2 = s3 = NULL;
         };
         send => {
             TokenList_append0(&token_list, TKSEND);
             s1 = s2 = s3 = NULL;
         };
         expect => {
             TokenList_append0(&token_list, TKEXPECT);
             s1 = s2 = s3 = NULL;
         };
         (ustring|qstring) => {
             TokenList_append1(&token_list, TKSTRING, s1);
             s1 = s2 = s3 = NULL;
         };
         pattern => {
             TokenList_append1(&token_list, TKPATTERN, s1);
             s1 = s2 = s3 = NULL;
         };
         number => {
             TokenList_append1(&token_list, TKNUMBER, s1);
             s1 = s2 = s3 = NULL;
         };
         url => {
             TokenList_append3(&token_list, TKURL, s1, s2, s3);
             s1 = s2 = s3 = NULL;
         };
         newline => {
             TokenList_append0(&token_list, TKNEWLINE);
             s1 = s2 = s3 = NULL;
         };
         dict_start => {
             TokenList_append0(&token_list, TKDICT_START);
             s1 = s2 = s3 = NULL;
         };
         dict_end => {
             TokenList_append0(&token_list, TKDICT_END);
             s1 = s2 = s3 = NULL;
         };
         equals => {
             TokenList_append0(&token_list, TKEQUALS);
             s1 = s2 = s3 = NULL;
         };
         space => { };
         ',' => { };
         comment => { };
    *|;
}%%

%% write data;

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

    %% write init;
    %% write exec;

    return token_list;

error:
    TokenList_destroy(token_list);
    return NULL;
}