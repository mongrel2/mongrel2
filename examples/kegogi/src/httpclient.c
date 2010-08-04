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
    else if (biseqcstrcaseless(bField, "transfer-encoding"))
        rsp->chunked_body = biseqcstrcaseless(bVal, "chunked");

    check(!Headers_add(&rsp->headers, bField, bVal), "Failed to add headers");

    return;
error:
    bdestroy(bField);
    bdestroy(bVal);
}

static void status_code(void *data, const char *at, size_t len)
{
    Response *rsp = (Response *) data;
    bstring bs = blk2bstr(at, len);
    rsp->status_code = atoi(bdata(bs));
    bdestroy(bs);
}

static void header_done(void *data, const char *at, size_t len)
{
    Response *rsp = (Response *) data;
    rsp->body_start = at;
    rsp->body_so_far = len;
}

static void element_nop(void *_, const char *__, size_t ___) { /* NOP */ }

Response *Response_fetch(Request *req) {
    int rc = 0;
    int fd = 0;
    char *buffer = malloc(FETCH_BUFFER_SIZE);
    check(buffer != NULL, "Failed to allocate fetch buffer");

    Response *rsp = malloc(sizeof(*rsp));
    check(rsp, "Failed to allocate response\n");
    Headers_init(&rsp->headers);
    rsp->content_len = -1;
    rsp->body = NULL;
    rsp->status_code = -1;
    rsp->chunked_body = 0;
    rsp->body = bfromcstr("");
    rsp->body_start = NULL;
    rsp->body_so_far = 0;

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
    

    ssize_t nread;

    while(!httpclient_parser_finish(&parser)) {
        nread = fdrecv(fd, buffer, FETCH_BUFFER_SIZE - 1);
        check(nread > 0, "Failed to get all of headers");
        buffer[nread] = '\0';
        size_t nparsed = httpclient_parser_execute(&parser, buffer, nread, 0);
    }
    check(httpclient_parser_finish(&parser) == 1, "Didn't parse.");

    if(rsp->chunked_body) {
        int i, r = 0, chunk_size = -1;
        int read_some_of_chunk_size = 0;

        if(rsp->body_start)
            i = rsp->body_start - buffer;
        else
            nread = i = 0;

        while(chunk_size != 0) {
            if(i >= nread) {
                // We skip past the end of the buffer to skip future characters
                // i.e. "|\r| end buffer |\n|..." we actually want to start at
                // index 1
                i = (i - nread);
                nread = fdrecv(fd, buffer, FETCH_BUFFER_SIZE);
                check(nread > 0, "Failed to fdrecv in chunk reading");
            }
            
            if(chunk_size == -1) {
                char c = toupper(buffer[i]);
                i++; // skip past this character

                if((c >= 'A' && c <= 'F') || (c >= '0' && c <= '9')) {
                    c = (c >= 'A') ? c - 'A' + 10 : c - '0';
                    r = (r * 16) + c;
                    read_some_of_chunk_size = 1;
                }
                else if(read_some_of_chunk_size) {
                    chunk_size = r;
                    read_some_of_chunk_size = r = 0;
                    i++; // Also skip over the \n
                }
            }
            else if(chunk_size <= nread - i) {
                int j;
                for(j = i; j < chunk_size + i; j++) printf("%c", buffer[j]);
                bcatblk(rsp->body, buffer + i, chunk_size);
                i += chunk_size;
                chunk_size = -1;
            }
            else { // chunk_size > nread - i
                int j;
                for(j = i; j < nread; j++) printf("%c", buffer[j]);
                bcatblk(rsp->body, buffer + i, nread - i);
                chunk_size -= (nread - i);
                i = nread;
            }
        }
    }
    else {
        if(rsp->content_len == -1) {
            while((nread = fdrecv(fd, buffer, FETCH_BUFFER_SIZE)) > 0)
                bcatblk(rsp->body, buffer, nread);
        }
        else if(rsp->content_len > 0) {
            ssize_t remaining = rsp->content_len - rsp->body_so_far;
            while(remaining > 0) {
                ssize_t toread = FETCH_BUFFER_SIZE;
                if(toread > remaining) toread = remaining;
                nread = fdrecv(fd, buffer, toread);
                check(nread > 0, "fdrecv failed with %d remaining", remaining);
                bcatblk(rsp->body, buffer, nread);
                remaining -= nread;
            }
            remaining -= rsp->body_so_far;
        }
    }

    debug("body length = %d", blength(rsp->body));

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
        Headers_cleanup(&rsp->headers);
        free(rsp);
    }
}
