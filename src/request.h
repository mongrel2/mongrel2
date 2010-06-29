#ifndef _request_h
#define _request_h

#include <http11/http11_parser.h>
#include <bstring.h>

http_parser *Request_create();

int Request_parse(http_parser *parser, char *buf, size_t nread,
        size_t *out_nparsed);

void Request_start(http_parser *parser);

void Request_destroy(http_parser *parser);

void Request_dump(http_parser *parser);

bstring Request_get(http_parser *parser, bstring field);

#endif
