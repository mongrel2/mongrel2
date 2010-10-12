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

#include <adt/tst.h>
#include <adt/list.h>
#include <host.h>
#include <routing.h>
#include <ssl/ssl.h>

enum {
     /* IPv6 addr can be up to 40 chars long */
    IPADDR_SIZE = 40
};

typedef struct Server {
    int port;
    int listen_fd;
    Host *default_host;
    RouteMap *hosts;
    bstring uuid;
    bstring chroot;
    bstring access_log;
    bstring error_log;
    bstring pid_file;
    bstring default_hostname;
    SSL_CTX *ssl_ctx;
} Server;


Server *Server_create(const char *uuid, const char *default_host,
        const char *port, const char *chroot, const char *access_log,
        const char *error_log, const char *pid_file);

void Server_destroy(Server *srv);

void Server_init();

void Server_start(Server *srv);

int Server_add_host(Server *srv, bstring pattern, Host *host);

void Server_set_default_host(Server *srv, Host *host);

Host *Server_match_backend(Server *srv, bstring target);

#endif
