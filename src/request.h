#ifndef _request_h
#define _request_h

#include <http11/http11_parser.h>
#include <adt/dict.h>
#include <bstring.h>
#include <handler.h>
#include <headers.h>
#include <host.h>

enum {
    REQUEST_EXTRA_HEADERS = 6
};

typedef struct Request {
    bstring request_method;
    bstring version;
    bstring uri;
    bstring path;
    bstring query_string;
    bstring fragment;
    Host *target_host;
    dict_t *headers;
    struct Backend *action;
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

#define Request_get_action(R, T) ((R)->action ? (R)->action->target.T : NULL)

#define Request_set_action(R, V) ((R)->action = (V))

#define Request_path(R) ((R)->path)

#define Request_content_length(R) ((R)->parser.content_len)

#define Request_header_length(R) ((R)->parser.body_start)

#endif
