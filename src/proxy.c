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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <dbg.h>
#include <proxy.h>
#include <assert.h>
#include <stdlib.h>
#include <mem/halloc.h>
#include <connection.h>
#include <http11/httpclient_parser.h>


void Proxy_destroy(Proxy *proxy)
{
    if(proxy) {
        if(proxy->server) bdestroy(proxy->server);
        h_free(proxy);
    }
}

Proxy *Proxy_create(bstring server, int port)
{
    Proxy *proxy = h_calloc(sizeof(Proxy), 1);
    check_mem(proxy);
    
    proxy->server = server;
    proxy->port = port;

    return proxy;

error:
    Proxy_destroy(proxy);
    return NULL;
}

const int PROXY_READ_RETRIES = 100;
const int PROXY_READ_RETRY_WARN = 10;

int Proxy_read_and_parse(Connection *conn)
{
    int avail = 0;
    int nparsed = 0;
    char *data = NULL;
    int tries = 0;

    assert(conn->client && "httpclient_parser not configured.");
    assert(conn->proxy_iob && "conn->proxy_iob not configured.");

    httpclient_parser_init(conn->client);

    for(tries = 0; tries < PROXY_READ_RETRIES; tries++) {
        data = IOBuf_read_some(conn->proxy_iob, &avail);
        check(data != NULL, "Failed to read from proxy.");

        data[avail] = '\0';
        nparsed = httpclient_parser_execute(conn->client, data, avail, nparsed);
        check(nparsed != -1, "Major parsing failure from proxy backend, Tell Zed.");
        check(!httpclient_parser_has_error(conn->client), "Parsing error from server.");

        if(httpclient_parser_finish(conn->client) == 0) {
            fdwait(conn->proxy_iob->fd, 'r');
        } else {
            break;
        }
    }

    if(tries > PROXY_READ_RETRY_WARN) {
        log_warn("The proxy backend has a bad I/O pattern that required %d reads before a header was completed. You should worry about its performance.", tries);
    }

    check(tries < PROXY_READ_RETRIES, "Backend proxy sends too many small packets, tried %d times to read from it.", tries);
    check(conn->client->body_start <= avail, "Read too much.");


    return nparsed;

error:
    return -1;
}


static inline int parse_chunks(char *data, httpclient_parser *client, int avail)
{
    // parse for the header
    data[avail] = '\0';
    httpclient_parser_init(client);
    int rc = httpclient_parser_execute(client, data, avail, 0);
    check(rc != -1, "Fatal parsing error from proxy, Tell Zed.");
    check(!httpclient_parser_has_error(client), "Parsing error from server.");
    check(httpclient_parser_finish(client), "Parser didn't get a full response.");

    return rc;
error:
    return -1;
}


int Proxy_stream_chunks(Connection *conn)
{
    int rc = 0;
    IOBuf *proxy_iob = conn->proxy_iob;
    assert(proxy_iob && "proxy IOBuf not configured.");

    int avail = IOBuf_avail(proxy_iob);
    char *data = NULL;
    httpclient_parser *client = conn->client;

    assert(client && "httpclient_parser not configured.");
    assert(client->chunked && "parser should indicate it has chunks.");
    assert(!client->chunks_done && "parser should not be in chunks done state.");
    assert(avail >= 0 && "Can't have a < 0 avail.");

    data = avail == 0 ? IOBuf_read_some(proxy_iob, &avail) : IOBuf_start(proxy_iob);

    // at this point we have something to work with and it's ready to parse
    rc = parse_chunks(data, client, avail);
    check_debug(rc != -1, "Failed to parse chunked encoding from client.");

    // remember the +2 is for the crlf at the end of the chunk body
    rc = IOBuf_stream(proxy_iob, conn->iob, client->body_start + client->content_len + 2);
    check_debug(rc != -1, "Failed to stream content to client.");

    return client->chunks_done;

error:
    return -1;
}


void Proxy_stream_to_close(Connection *conn)
{
    conn->client->close = 1;

    int total = conn->iob->len <= conn->proxy_iob->len ? 
        conn->iob->len : conn->proxy_iob->len;

    int rc = IOBuf_send_all(conn->iob, IOBuf_start(conn->proxy_iob),
            IOBuf_avail(conn->proxy_iob));

    if(rc == -1) return;

    IOBuf_read_commit(conn->proxy_iob, IOBuf_avail(conn->proxy_iob));

    for(rc = 0; rc != -1;) {
        rc = IOBuf_stream(conn->proxy_iob, conn->iob, total);
    }
}
