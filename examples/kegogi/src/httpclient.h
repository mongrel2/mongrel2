#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

#include <bstring.h>

#define MAX_NUM_HEADERS 128

typedef struct Header {
    bstring field;
    bstring value;
} Header;

typedef struct Headers {
    int num_headers;
    Header headers[MAX_NUM_HEADERS];
} Headers;

typedef struct Request {
    bstring host;
    int port;
    bstring method;
    bstring uri;
    Headers headers;
} Request;

typedef struct Response {
    bstring status_code;
    bstring body;
    Headers headers;
    int content_len;
    int body_so_far;
} Response;

int Headers_init(Headers *headers);
int Headers_cleanup(Headers *headers);
int Headers_add(Headers *headers, bstring field, bstring value);

Request *Request_create(bstring host, int port, bstring method, bstring uri);
void Request_destroy(Request *req);

Response *Response_create(bstring status_code);
Response *Response_fetch(Request *req);
void Response_destroy(Response *rsp);


#endif
