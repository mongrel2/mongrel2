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

#ifndef _server_h
#define _server_h

#include "adt/tst.h"
#include "adt/darray.h"
#include "host.h"
#include "routing.h"
#include <polarssl/ssl.h>
#include <polarssl/x509.h>
#include <polarssl/pk.h>

enum {
     /* IPv6 addr can be up to 40 chars long */
    IPADDR_SIZE = 40
};

extern darray_t *SERVER_QUEUE;

typedef struct Server {
    int port;
    int listen_fd;
    Host *default_host;
    RouteMap *hosts;
    darray_t *handlers;
    bstring bind_addr;
    bstring uuid;
    bstring chroot;
    bstring access_log;
    bstring error_log;
    bstring pid_file;
    bstring control_port;
    bstring default_hostname;
    uint32_t created_on;
    int use_ssl;
    x509_crt own_cert;
    x509_crt ca_chain;
    pk_context pk_key;
    const int *ciphers;
    char *dhm_P;
    char *dhm_G;
} Server;

Server *Server_create(bstring uuid, bstring default_host,
        bstring bind_addr, int port, bstring chroot,
        bstring access_log, bstring error_log, bstring pid_file,
        bstring control_port, int use_ssl);

void Server_destroy(Server *srv);

void Server_init();

int Server_run();

int Server_add_host(Server *srv, Host *host);

void Server_set_default_host(Server *srv, Host *host);

Host *Server_match_backend(Server *srv, bstring target);

int Server_start_handlers(Server *srv, Server *copy_from);

int Server_stop_handlers(Server *srv);

int Server_queue_init();

void Server_queue_cleanup();

void Server_queue_push(Server *srv);

#define Server_queue_latest() darray_last(SERVER_QUEUE)

int Server_queue_destroy();

#endif
