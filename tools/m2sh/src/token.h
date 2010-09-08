#ifndef _TOKENS_H
#define _TOKENS_H

#include <bstring.h>
#include <adt/list.h>
#include <adt/hash.h>

typedef int TokenType;

typedef struct Token {
    TokenType type;
    bstring data;
} Token;

list_t *Lexer_tokenize(bstring content);


void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *p, void (*freeProc)(void*));

void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  Token *yyminor,       /* The value for the token */
  hash_t **out_settings
);

#endif
