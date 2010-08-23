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

#ifndef _listener_h
#define _listener_h

#include <server.h>
#include <request.h>
#include <state.h>
#include <proxy.h>

enum
{
	CONNECTION_STACK = 32 * 1024,
    BUFFER_SIZE = 2 * 1024
};


typedef struct Connection {
    Server *server;
    int fd;
    int proxy_fd;
    Request *req;
    int nread;
    size_t nparsed;
    int finished;
    int registered;
    int rport;
    State state;
    char *buf;
    char *proxy_buf;
    struct httpclient_parser *client;
    char remote[IPADDR_SIZE+1];
} Connection;

void Connection_destroy(Connection *conn);

Connection *Connection_create(Server *srv, int fd, int rport, const char *remote);

void Connection_accept(Connection *conn);

void Connection_task(void *v);


int Connection_deliver_raw(int to_fd, bstring buf);
int Connection_deliver(int to_fd, bstring buf);

int Connection_read_header(Connection *conn, Request *req);

#endif
