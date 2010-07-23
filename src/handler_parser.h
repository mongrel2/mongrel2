#ifndef _handler_parser_h
#define _handler_parser_h

#include <stdlib.h>
#include <bstring.h>

enum {
    MAX_TARGETS = 128
};

typedef struct HandlerParser {
    const char *body_start;
    size_t body_length;
    bstring uuid;

    size_t target_count;
    unsigned long targets[MAX_TARGETS];
} HandlerParser;


int HandlerParser_execute(HandlerParser *parser, const char *buffer, size_t len);

#endif
