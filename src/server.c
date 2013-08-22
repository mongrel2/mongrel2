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

#include "dbg.h"
#include "task/task.h"
#include "connection.h"
#include "register.h"
#include "server.h"
#include "host.h"
#include <assert.h>
#include <string.h>
#include "mem/halloc.h"
#include "routing.h"
#include "setting.h"
#include "pattern.h"
#include "config/config.h"
#include "unixy.h"
#include <signal.h>

darray_t *SERVER_QUEUE = NULL;
int RUNNING=1;

static char *ssl_default_dhm_P = 
    "E4004C1F94182000103D883A448B3F80" \
    "2CE4B44A83301270002C20D0321CFD00" \
    "11CCEF784C26A400F43DFB901BCA7538" \
    "F2C6B176001CF5A0FD16D2C48B1D0C1C" \
    "F6AC8E1DA6BCC3B4E1F96B0564965300" \
    "FFA1D0B601EB2800F489AA512C4B248C" \
    "01F76949A60BB7F00A40B1EAB64BDD48" \
    "E8A700D60B7F1200FA8E77B0A979DABF";

static char *ssl_default_dhm_G = "4";

typedef struct CipherName
{
    char *name;
    int id;
} CipherName;

static CipherName cipher_table[] =
{
    { "SSL_RSA_RC4_128_MD5",          TLS_RSA_WITH_RC4_128_MD5 },
    { "SSL_RSA_RC4_128_SHA",          TLS_RSA_WITH_RC4_128_SHA },
    { "SSL_RSA_DES_168_SHA",          TLS_RSA_WITH_3DES_EDE_CBC_SHA },
    { "SSL_EDH_RSA_DES_168_SHA",      TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA },
    { "SSL_RSA_AES_128_SHA",          TLS_RSA_WITH_AES_128_CBC_SHA },
    { "SSL_EDH_RSA_AES_128_SHA",      TLS_DHE_RSA_WITH_AES_128_CBC_SHA },
    { "SSL_RSA_AES_256_SHA",          TLS_RSA_WITH_AES_256_CBC_SHA },
    { "SSL_EDH_RSA_AES_256_SHA",      TLS_DHE_RSA_WITH_AES_256_CBC_SHA },
    { "SSL_RSA_CAMELLIA_128_SHA",     TLS_RSA_WITH_CAMELLIA_128_CBC_SHA },
    { "SSL_EDH_RSA_CAMELLIA_128_SHA", TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA },
    { "SSL_RSA_CAMELLIA_256_SHA",     TLS_RSA_WITH_CAMELLIA_256_CBC_SHA },
    { "SSL_EDH_RSA_CAMELLIA_256_SHA", TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA },
    { NULL, -1 }
};

void host_destroy_cb(Route *r, RouteMap *map)
{
    (void)map;

    if(r->data) {
        Host_destroy((Host *)r->data);
        r->data = NULL;
    }
}

static int Server_load_ciphers(Server *srv, bstring ssl_ciphers_val)
{
    struct bstrList *ssl_cipher_list = bsplit(ssl_ciphers_val, ' ');
    int i = 0, n = 0;
    int max_num_ciphers = 0;
    int *ciphers = NULL;
    const int *default_ciphersuites;

    check(ssl_cipher_list != NULL && ssl_cipher_list->qty > 0,
            "Invalid cipher list, it must be separated by space ' ' characters "
            "and you need at least one.  Or, just leave it out for defaults.");

    default_ciphersuites = ssl_list_ciphersuites();
    while(default_ciphersuites[max_num_ciphers] != 0) {
        max_num_ciphers++;
    }

    ciphers = h_calloc(max_num_ciphers + 1, sizeof(int));
    check_mem(ciphers);

    for(i = 0; i < ssl_cipher_list->qty; i++) {
        bstring cipher = ssl_cipher_list->entry[i];

        int id = -1;
        for(n = 0; cipher_table[n].name != NULL; ++n)
        {
            if(biseqcstr(cipher, cipher_table[n].name))
            {
                id = cipher_table[n].id;
                break;
            }
        }

        if(id != -1)
            ciphers[i] = id;
        else
            sentinel("Unrecognized cipher: %s", bdata(cipher));
    }

    bstrListDestroy(ssl_cipher_list);
    ciphers[i] = 0;
    srv->ciphers = ciphers;
    hattach(ciphers, srv);

    return 0;
error:
    if(ssl_cipher_list) bstrListDestroy(ssl_cipher_list);
    if(ciphers != NULL) h_free(ciphers);
    return -1;
}


static int Server_init_ssl(Server *srv)
{
    int rc = 0;
    bstring certdir = NULL;
    bstring certpath = NULL;
    bstring keypath = NULL;

    bstring certdir_setting = Setting_get_str("certdir", NULL);
    check(certdir_setting != NULL, "to use ssl, you must specify a certdir");

    if(srv->chroot != NULL && !Unixy_in_chroot()) {
        certdir = bformat("%s%s", bdata(srv->chroot), bdata(certdir_setting));
    } else {
        certdir = bstrcpy(certdir_setting);
    }

    certpath = bformat("%s%s.crt", bdata(certdir), bdata(srv->uuid)); 
    check_mem(certpath);

    keypath = bformat("%s%s.key", bdata(certdir), bdata(srv->uuid));
    check_mem(keypath);

    rc = x509_crt_parse_file(&srv->own_cert, bdata(certpath));
    check(rc == 0, "Failed to load cert from %s", bdata(certpath));

    rc = pk_parse_keyfile(&srv->pk_key, bdata(keypath), NULL);
    check(rc == 0, "Failed to load key from %s", bdata(keypath));

    bstring ssl_ciphers_val = Setting_get_str("ssl_ciphers", NULL);
    
    bstring ca_chain = Setting_get_str("ssl.ca_chain", NULL);

    if ( ca_chain != NULL ) {

        rc = x509_crt_parse_file(&srv->ca_chain, bdata(ca_chain));
        check(rc == 0, "Failed to load cert from %s", bdata(ca_chain));

    } else {
        
        //to indicate no ca_chain was loaded
        srv->ca_chain.version=-1;
    }

    if(ssl_ciphers_val != NULL) {
        rc = Server_load_ciphers(srv, ssl_ciphers_val);
        check(rc == 0, "Failed to load requested SSL ciphers.");
    } else {
        srv->ciphers = ssl_list_ciphersuites();
    }

    srv->dhm_P = ssl_default_dhm_P;
    srv->dhm_G = ssl_default_dhm_G;

    bdestroy(certdir);
    bdestroy(certpath);
    bdestroy(keypath);

    return 0;

error:
    // Do not free certdir_setting, as we're pulling it from Settings
    if(certdir != NULL) bdestroy(certdir);
    if(certpath != NULL) bdestroy(certpath);
    if(keypath != NULL) bdestroy(keypath);
    return -1;
}

Server *Server_create(bstring uuid, bstring default_host,
        bstring bind_addr, int port, bstring chroot, bstring access_log,
        bstring error_log, bstring pid_file, bstring control_port, int use_ssl)
{
    Server *srv = NULL;
    int rc = 0;

    srv = h_calloc(sizeof(Server), 1);
    check_mem(srv);

    srv->hosts = RouteMap_create(host_destroy_cb);
    check(srv->hosts != NULL, "Failed to create host RouteMap.");

    srv->handlers = darray_create(sizeof(Handler), 20);
    check_mem(srv->handlers);

    check(port > 0, "Invalid port given, must be > 0: %d", port);
    srv->port = port;
    srv->listen_fd = 0;

    srv->bind_addr = bstrcpy(bind_addr); check_mem(srv->bind_addr);
    srv->uuid = bstrcpy(uuid); check_mem(srv->uuid);
    if(blength(chroot) > 0) {
        srv->chroot = bstrcpy(chroot); check_mem(srv->chroot);
    } else {
        srv->chroot = NULL;
    }
    srv->access_log = bstrcpy(access_log); check_mem(srv->access_log);
    srv->error_log = bstrcpy(error_log); check_mem(srv->error_log);
    srv->pid_file = bstrcpy(pid_file); check_mem(srv->pid_file);
    if(blength(control_port) > 0) {
        srv->control_port = bstrcpy(control_port); check_mem(srv->control_port);
    } else {
        srv->control_port = NULL;
    }
    srv->default_hostname = bstrcpy(default_host);
    srv->use_ssl = use_ssl;
    srv->created_on = time(NULL);

    if(srv->use_ssl) {
        rc = Server_init_ssl(srv);
        check(rc == 0, "Failed to initialize ssl for server %s", bdata(uuid));
    }

    return srv;

error:
    Server_destroy(srv);
    return NULL;
}


static inline void Server_destroy_handlers(Server *srv)
{
    int i = 0;
    for(i = 0; i < darray_end(srv->handlers); i++) {
        Handler *handler = darray_get(srv->handlers, i);
        Handler_destroy(handler);
    }

    darray_destroy(srv->handlers);
}


void Server_destroy(Server *srv)
{
    if(srv) {
        if(srv->use_ssl) {
            x509_crt_free(&srv->own_cert);
            pk_free(&srv->pk_key);
            // srv->ciphers freed (if non-default) by h_free
        }

        RouteMap_destroy(srv->hosts);
        Server_destroy_handlers(srv);

        bdestroy(srv->bind_addr);
        bdestroy(srv->uuid);
        bdestroy(srv->chroot);
        bdestroy(srv->access_log);
        bdestroy(srv->error_log);
        bdestroy(srv->pid_file);
        bdestroy(srv->control_port);
        bdestroy(srv->default_hostname);

        if(srv->listen_fd >= 0) fdclose(srv->listen_fd);
        h_free(srv);
    }
}


void Server_init()
{
    int mq_threads = Setting_get_int("zeromq.threads", 1);

    if(mq_threads > 1) {
        log_info("WARNING: Setting zeromq.threads greater than 1 can cause lockups in your handlers.");
    }

    log_info("Starting 0MQ with %d threads.", mq_threads);
    mqinit(mq_threads);
    Register_init();
    Request_init();
    Connection_init();
}


static inline int Server_accept(int listen_fd)
{
    int cfd = -1;
    int rport = -1;
    char remote[IPADDR_SIZE];
    int rc = 0;
    Connection *conn = NULL;

    cfd = netaccept(listen_fd, remote, &rport);
    check(cfd >= 0, "Failed to accept on listening socket.");

    Server *srv = Server_queue_latest();
    check(srv != NULL, "Failed to get a server from the configured queue.");

    conn = Connection_create(srv, cfd, rport, remote);
    check(conn != NULL, "Failed to create connection after accept.");

    rc = Connection_accept(conn);
    check(rc == 0, "Failed to register connection, overloaded.");

    return 0;

error:

    if(conn != NULL) Connection_destroy(conn);
    return -1;
}


int Server_run()
{
    int rc = 0;
    Server *srv = Server_queue_latest();
    check(srv != NULL, "Failed to get a server from the configured queue.");
    int listen_fd = srv->listen_fd;

    taskname("SERVER");

    while(RUNNING) {
        rc = Server_accept(listen_fd);

        if(rc == -1 && RUNNING) {
            log_err("Server accept failed, attempting to clear out dead weight: %d", RUNNING);
            int cleared = Register_cleanout();

            if(cleared == 0) {
                taskdelay(1000);
            }
        }
    }

    return 0;
error:
    return -1;
}


int Server_add_host(Server *srv, Host *host)
{
    check(host->matching != NULL, "Host's matching can't be NULL.");
    return RouteMap_insert_reversed(srv->hosts, host->matching, host);

error:
    return -1;
}


void Server_set_default_host(Server *srv, Host *host)
{
    srv->default_host = host;
}



Host *Server_match_backend(Server *srv, bstring target)
{
    check(srv != NULL,  "Server is NULL?!");

    if(srv->default_host) {
        check(srv->default_host->matching != NULL,  "Server has a default_host without matching.");
    }

    debug("Looking for target host: %s", bdata(target));
    Route *found = RouteMap_match_suffix(srv->hosts, target);

    return found != NULL ? found->data : srv->default_host;

error:
    return NULL;
}

static inline int same_handler(Handler *from, Handler *to)
{
    return biseq(from->send_ident, to->send_ident) && 
        biseq(from->recv_ident, to->recv_ident) &&
        biseq(from->recv_spec, to->recv_spec) &&
        biseq(from->send_spec, to->send_spec);
}

typedef struct RouteUpdater {
    Handler *original;
    Handler *replacement;
} RouteUpdater;


static void update_routes(void *value, void *data)
{
    RouteUpdater *update = data;
    Backend *backend = ((Route *)value)->data;

    if(backend->type == BACKEND_HANDLER && backend->target.handler == update->original) {
        debug("Found backend that needs replacing: %p replaced with %p",
                update->original, update->replacement);
        backend->target.handler = update->replacement;
    }
}

static void update_host_routes(void *value, void *data)
{
    Host *host = ((Route *)value)->data;
    tst_traverse(host->routes->routes, update_routes, data);
}

static inline int Server_copy_active_handlers(Server *srv, Server *copy_from)
{
    int i = 0;
    for(i = 0; i < darray_end(copy_from->handlers); i++) {
        Handler *from = darray_get(copy_from->handlers, i);

        int j = 0;
        for(j = 0; j < darray_end(srv->handlers); j++) {
            Handler *to = darray_get(srv->handlers, j);

            if(same_handler(from, to))
            {
                debug("Swapping %p original for %p replacement", to, from);
                RouteUpdater update = {.original = to, .replacement = from};
                tst_traverse(srv->hosts->routes, update_host_routes, &update);

                darray_set(srv->handlers, j, from);
                // swap them around so that the darrays stay full
                darray_set(copy_from->handlers, i, to);
                to->running = 0;
                break;
            }
        }
    }

    return 0;
}

int Server_start_handlers(Server *srv, Server *copy_from)
{
    int i = 0;
    int rc = 0;

    if(copy_from != NULL) {
        rc = Server_copy_active_handlers(srv, copy_from);
        check(rc != -1, "Failed to copy old handlers to new server config.");
    }

    for(i = 0; i < darray_end(srv->handlers); i++) {
        Handler *handler = darray_get(srv->handlers, i);
        check(handler != NULL, "Invalid handler, can't be NULL.");

        if(!handler->running) {
            log_info("LOADING Handler %s", bdata(handler->send_spec));
            rc = taskcreate(Handler_task, handler, HANDLER_STACK);
            check(rc != -1, "Failed to start handler task.");
            handler->running = 1;
        }
    }

    return 0;
error:
    return -1;
}


int Server_stop_handlers(Server *srv)
{
    int i = 0;
    for(i = 0; i < darray_end(srv->handlers); i++) {
        Handler *handler = darray_get(srv->handlers, i);
        check(handler != NULL, "Invalid handler, can't be NULL.");

        if(handler->running) {
            log_info("STOPPING HANDLER %s", bdata(handler->send_spec));
            if(handler->task != NULL) {
                tasksignal(handler->task, SIGINT);
                handler->running = 0;
                taskdelay(1);
            }
        }

        if(handler->recv_socket) zmq_close(handler->recv_socket);
        if(handler->send_socket) zmq_close(handler->send_socket);
        handler->recv_socket = NULL;
        handler->send_socket = NULL;
    }

    return 0;
error:
    return -1;
}


int Server_queue_init()
{
    SERVER_QUEUE = darray_create(sizeof(Server), 100);
    check(SERVER_QUEUE != NULL, "Failed to create server management queue.");
    return 0;
error:
    return -1;
}

const int SERVER_TTL = 10;
const int SERVER_ACTIVE = 5;

void Server_queue_cleanup()
{
    if(darray_end(SERVER_QUEUE) < SERVER_ACTIVE) {
        // skip it, not enough to care about
        return;
    }

    // pop the last one off to make sure it's never deleted
    Server *cur_srv = darray_pop(SERVER_QUEUE);
    uint32_t too_old = time(NULL) - SERVER_TTL;
    int i = 0;

    // TODO: kind of a dumb way to do this since it reorders the list
    // go through all but the max we want to keep
    for(i = 0; i < darray_end(SERVER_QUEUE) - SERVER_ACTIVE; i++) {
        Server *srv = darray_get(SERVER_QUEUE, i);

        if(srv->created_on < too_old) {
            Server *replace = darray_pop(SERVER_QUEUE);
            darray_set(SERVER_QUEUE, i, replace);

            srv->listen_fd = -1; // don't close it
            Server_destroy(srv);
        }
    }

    // put the sacred server back on the end
    darray_push(SERVER_QUEUE, cur_srv);

    return;
}


void Server_queue_push(Server *srv)
{
    Server_queue_cleanup();
    darray_push(SERVER_QUEUE, srv);
    hattach(srv, SERVER_QUEUE);
}


int Server_queue_destroy()
{
    int i = 0;
    Server *srv = NULL;

    for(i = 0; i < darray_end(SERVER_QUEUE); i++) {
        srv = darray_get(SERVER_QUEUE, i);
        check(srv != NULL, "Got a NULL value from the server queue.");
        Server_destroy(srv);
    }

    darray_destroy(SERVER_QUEUE);

    return 0;
error:
    return -1;
}
