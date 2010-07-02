#ifndef _request_h
#define _request_h

#include <http11/http11_parser.h>
#include <adt/dict.h>
#include <bstring.h>
#include <handler.h>

typedef struct Request {
    bstring request_method;
    bstring version;
    bstring uri;
    bstring path;
    bstring query_string;
    bstring fragment;
    dict_t *headers;
    struct Backend *action;
    int invalid_headers;

    http_parser parser;
} Request;

Request *Request_create();

int Request_parse(Request *parser, char *buf, size_t nread, size_t *out_nparsed);

void Request_start(Request *parser);

void Request_destroy(Request *parser);

void Request_dump(Request *parser);

bstring Request_get(Request *parser, bstring field);

#define Request_parser(R) (&((R)->parser))

#define Request_is_json(R) ((R)->parser.json_sent == 1)

#define Request_is_socket(R) ((R)->parser.socket_started == 1)

#define Request_is_http(R) (!(Request_is_json(R) || Request_is_socket(R)))

#endif
