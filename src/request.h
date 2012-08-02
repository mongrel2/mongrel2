/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _request_h
#define _request_h

#include <http11/http11_parser.h>
#include <adt/hash.h>
#include <bstring.h>
#include <handler.h>
#include <headers.h>
#include <host.h>
struct Connection;

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
    bstring host; // not owned by us
    bstring host_name;
    bstring pattern; // not owned by us
    bstring prefix; // not owned by us
    struct Host *target_host;
    hash_t *headers;
    struct Backend *action;
    int status_code;
    int response_size;
    unsigned ws_flags;
    bstring new_header;
    http_parser parser;
} Request;

Request *Request_create();

int Request_parse(Request *req, char *buf, size_t nread, size_t *out_nparsed);

void Request_start(Request *req);

void Request_destroy(Request *req);

bstring Request_get(Request *req, bstring field);

void Request_set(Request *req, bstring key, bstring val, int replace);

int Request_get_date(Request *req, bstring field, const char *format);

#define Request_parser(R) (&((R)->parser))

#define Request_is_json(R) ((R)->parser.json_sent == 1)

#define Request_is_xml(R) ((R)->parser.xml_sent == 1)

#define Request_is_http(R) (!(Request_is_json(R) || Request_is_xml(R)))

#define Request_get_action(R, T) ((R)->action ? (R)->action->target.T : NULL)

#define Request_set_action(R, V) ((R)->action = (V))

#define Request_path(R) ((R)->path)

#define Request_content_length(R) ((R)->parser.content_len)

#define Request_header_length(R) ((R)->parser.body_start)

bstring Request_to_tnetstring(Request *req, bstring uuid, int fd, const char *buf, size_t len, struct Connection *);

bstring Request_to_payload(Request *req, bstring uuid, int fd, const char *buf, size_t len, struct Connection *);

void Request_init();

#endif
