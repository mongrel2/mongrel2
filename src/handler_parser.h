#ifndef _handler_parser_h
#define _handler_parser_h

#include <stdlib.h>
#include <bstring.h>

typedef struct HandlerParser {
    const char *body_start;
    size_t body_length;
    bstring uuid;

    size_t target_count;
    size_t target_max;
    unsigned long *targets;
} HandlerParser;

HandlerParser *HandlerParser_create(size_t max_targets);

void HandlerParser_destroy(HandlerParser *parser);

int HandlerParser_execute(HandlerParser *parser, const char *buffer, size_t len);

void HandlerParser_reset(HandlerParser *parser);

#endif
