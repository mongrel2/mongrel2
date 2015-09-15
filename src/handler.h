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

#ifndef _handler_h
#define _handler_h

#include <stdlib.h>
#include <bstring.h>
#include <task/task.h>

extern int HANDLER_STACK;

typedef enum { HANDLER_PROTO_JSON, HANDLER_PROTO_TNET } handler_protocol_t;

typedef struct Handler {
    void *send_socket;
    void *recv_socket;
    bstring send_ident;
    bstring recv_ident;
    bstring recv_spec;
    bstring send_spec;
    Task *task;
    int running;
    int raw;
    handler_protocol_t protocol;
} Handler;

void Handler_task(void *v);

int Handler_deliver(void *handler_socket, char *buffer, size_t len);

Handler *Handler_create(bstring send_spec, bstring send_ident,
        bstring recv_spec, bstring recv_ident);

void Handler_destroy(Handler *handler);

void *Handler_recv_create(const char *recv_spec, const char *uuid);

void *Handler_send_create(const char *send_spec, const char *identity);

void Handler_notify_leave(Handler *handler, int id);

void Handler_notify_credits(Handler *handler, int id, int credits);


#endif
