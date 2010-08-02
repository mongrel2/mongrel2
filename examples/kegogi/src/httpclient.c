#include "httpclient.h"

#include <stdlib.h>

#include <task/task.h>
#include <bstring.h>
#include <dbg.h>

#include "kegogi.h"
#include "httpclient_parser.h"

#define FETCH_BUFFER_SIZE 10 * 1024

char *REQUEST_FORMAT = 
    "%s %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n"
    "\r\n";

int Headers_init(Headers *headers) {
    if(!headers) return -1;
    headers->num_headers = 0;
    return 0;
}

int Headers_cleanup(Headers *headers) {
    int i;
    for(i = 0; i < headers->num_headers; i++) {
        bdestroy(headers->headers[i].field);
        bdestroy(headers->headers[i].value);
    }
    return 0;
}

int Headers_add(Headers *headers, bstring field, bstring value) {
    if(!headers || headers->num_headers >= MAX_NUM_HEADERS) return -1;
    int i = headers->num_headers;
    headers->headers[i].field = field;
    headers->headers[i].value = value;
    headers->num_headers++;
    return 0;
}

Request *Request_create(bstring host, int port, bstring method, bstring uri) {
    Request *req = malloc(sizeof(*req));
    req->host = host;
    req->port = port;
    req->method = method;
    req->uri = uri;
    Headers_init(&req->headers);
    req->headers.num_headers = 0;
    return req;
}

static void http_field(void *data, const char *field, size_t flen,
                       const char *val, size_t vlen)
{
    Response *rsp = (Response *) data;
    
    bstring bField = blk2bstr(field, flen);
    bstring bVal = blk2bstr(val, vlen);

    if(biseqcstrcaseless(bField, "content-length"))
        rsp->content_len = atoi(bdata(bVal));

    check(!Headers_add(&rsp->headers, bField, bVal), "Failed to add headers");

    return;
error:
    bdestroy(bField);
    bdestroy(bVal);
}

static void status_code(void *data, const char *at, size_t len)
{
    Response *rsp = (Response *) data;
    rsp->status_code = blk2bstr(at, len);
    bconchar(rsp->status_code, '\0');
}

static void header_done(void *data, const char *at, size_t len)
{
    Response *rsp = (Response *) data;
    rsp->body_so_far = len;
}

static void element_nop(void *_, const char *__, size_t ___) { /* NOP */ }

static void dump_remaining_from_fd(int fd, ssize_t remaining)
{
    char *buffer = malloc(FETCH_BUFFER_SIZE);
    if(remaining > 0) {
        while(remaining > 0) {
            int toread = FETCH_BUFFER_SIZE;
            if(toread > remaining) toread = remaining;
            int nread = fdrecv(fd, buffer, toread);
            check(nread, "fdrecv failed unexpectedly with %d remaining",
                  remaining);
            remaining -= nread;
        }
    }
    else if(remaining == -1) {
        while(fdrecv(fd, buffer, sizeof(buffer)) > 0)
            continue;
    }
    return;
error:
    return;
}

Response *Response_fetch(Request *req) {
    int rc = 0;
    int fd = 0;
    char *buffer = malloc(FETCH_BUFFER_SIZE);
    check(buffer != NULL, "Failed to allocate fetch buffer");

    Response *rsp = malloc(sizeof(*rsp));
    check(rsp, "Failed to allocate response\n");
    Headers_init(&rsp->headers);
    rsp->content_len = -1;
    rsp->body_so_far = 0;
    rsp->body = NULL;
    rsp->status_code = NULL;

    httpclient_parser parser;
    rc = httpclient_parser_init(&parser);
    check(rc, "Failed to init parser.");
    parser.http_field = http_field;
    parser.reason_phrase = element_nop;
    parser.status_code = status_code;
    parser.chunk_size = element_nop;
    parser.http_version = element_nop;
    parser.header_done = header_done;
    parser.last_chunk = element_nop;
    parser.data = rsp;

    fd = netdial(TCP, bdata(req->host), req->port);
    check(fd > 0, "Failed to connect to %s on port %d.", bdata(req->host),
          req->port);

    bstring request =  bformat(REQUEST_FORMAT, bdata(req->method),
                               bdata(req->uri), bdata(req->host));
    int totalsent = fdsend(fd, bdata(request), blength(request));
    check(totalsent == blength(request), "Didn't send all of the request.");
    
    while(!httpclient_parser_finish(&parser)) {
        int nread = fdrecv(fd, buffer, FETCH_BUFFER_SIZE - 1);
        check(nread > 0, "Failed to get all of headers");
        buffer[nread] = '\0';
        size_t nparsed = httpclient_parser_execute(&parser, buffer, nread, 0);
    }
    check(httpclient_parser_finish(&parser) == 1, "Didn't parse.");

    ssize_t remaining = rsp->content_len;
    if(remaining > 0) remaining -= rsp->body_so_far;

    dump_remaining_from_fd(fd, remaining);

    return rsp;

error:
    bdestroy(request);
    return NULL;
}

void Request_destroy(Request *req) {
    if(req) {
        bdestroy(req->host);
        bdestroy(req->uri);
        bdestroy(req->method);
        Headers_cleanup(&req->headers);
        free(req);
    }
}

void Response_destroy(Response *rsp) {
    if(rsp) {
        if(rsp->body) bdestroy(rsp->body);
        if(rsp->status_code) bdestroy(rsp->status_code);
        Headers_cleanup(&rsp->headers);
        free(rsp);
    }
}
