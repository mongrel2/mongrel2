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
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>
#include <ctype.h>
#include <mbedtls/sha1.h>

#include "connection.h"
#include "http11/httpclient_parser.h"
#include "dbg.h"
#include "events.h"
#include "register.h"
#include "pattern.h"
#include "dir.h"
#include "response.h"
#include "mem/halloc.h"
#include "setting.h"
#include "log.h"
#include "upload.h"
#include "filter.h"
#include "websocket.h"
#include "adt/darray.h"
#include "adt/dict.h"
#include "headers.h"


struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":\\s*\"ping\"}");

struct tagbstring POLICY_XML_REQUEST = bsStatic("<policy-file-request");

int MAX_CONTENT_LENGTH = 20 * 1024;
int MAX_CHUNK_SIZE = 100 * 1024;
int BUFFER_SIZE = 4 * 1024;
int MAX_CREDITS = 100 * 1024;
int CONNECTION_STACK = 32 * 1024;
int CLIENT_READ_RETRIES = 5;
int RELAXED_PARSING = 0;
int DOWNLOAD_FLOW_CONTROL = 0;


static dict_t* sni_cache;

int MAX_WS_LENGTH = 256 * 1024;


static void free_lnode(list_t *l, lnode_t *i, void *data)
{
    (void)l;
    (void)data;
    free(lnode_get(i));
}


static inline int Connection_backend_event(Backend *found, Connection *conn)
{
    switch(found->type) {
        case BACKEND_HANDLER:
            return HANDLER;
        case BACKEND_DIR:
            return DIRECTORY;
        case BACKEND_PROXY:
            return PROXY;
        default:
            error_response(conn, 501, "Invalid backend type: %d", found->type);
    }

error:
    return CLOSE;
}


static int Connection_handler_notify_leave(Connection *conn)
{
    int fd;
    uint32_t id;

    fd = IOBuf_fd(conn->iob);
    id = Register_id_for_fd(fd);
    check_debug(id != UINT32_MAX, "Asked to write to an fd that doesn't exist: %d", fd);

    check(conn->handler != NULL, "No handler to notify");

    Handler_notify_leave(conn->handler, id);

    return 0;

error:
    return -1;
}


int Connection_deliver_enqueue(Connection *conn, deliver_function f,
                                             tns_value_t *d)
{
    int dataSize = (d ? blength(d->value.string) : 0);
    Deliver_message *m = NULL;
    lnode_t *i = NULL;

    if(DOWNLOAD_FLOW_CONTROL) {
        check_debug(conn->deliverBytesPending + dataSize <= MAX_CREDITS, "Too many outstanding bytes");
    } else {
        check_debug(list_count(conn->deliverQueue) < DELIVER_OUTSTANDING_MSGS, "Too many outstanding messages");
    }

    check_debug(conn->deliverTaskStatus==DT_RUNNING, "Cannot enqueue, deliver task not running");

    m = (Deliver_message *)calloc(sizeof(Deliver_message), 1);
    check_mem(m);

    m->deliver = f;
    m->data = d;

    i = lnode_create(m);
    check_mem(i);

    list_append(conn->deliverQueue, i);

    conn->deliverBytesPending += dataSize;
    taskwakeup(&conn->deliverRendez);
    return 0;

error:
    if(m != NULL) {
        free(m);
    }
    if(i != NULL) {
        lnode_destroy(i);
    }
    return -1;
}

static inline void Connection_deliver_dequeue(Connection *conn, Deliver_message *m)
{
    lnode_t *i;
    Deliver_message *idata;
    int dataSize;

    while(1) {
        if(!list_isempty(conn->deliverQueue)) {
            i = list_del_first(conn->deliverQueue);
            idata = (Deliver_message *)lnode_get(i);
            *m = *idata;
            lnode_destroy(i);
            free(idata);
            dataSize = (m->data ? blength(m->data->value.string) : 0);
            conn->deliverBytesPending -= dataSize;
            return;
        }
        tasksleep(&conn->deliverRendez);
    }
}

int connection_send_socket_response(Connection *conn)
{
    check(Response_send_socket_policy(conn) > 0, "Failed to send Flash cross domain policy.");

error: // fallthrough, because flash expects it to close
    return CLOSE;
}


int connection_route_request(Connection *conn)
{
    Host *host = NULL;
    Route *route = NULL;

    bstring path = Request_path(conn->req);
    check_debug(path != NULL, "No path given, in request, ignoring.");

    Server *server = Server_queue_latest();
    check(server != NULL, "No server in the server queue, tell Zed.");

    if(conn->req->host_name) {
        host = Server_match_backend(server, conn->req->host_name);
    } else {
        host = server->default_host;
    }

    error_unless(host, conn, 404, "Request for a host we don't have registered: %s", bdata(conn->req->host_name));

    Backend *found = Host_match_backend(host, path, &route);
    error_unless(found, conn, 404, "Handler not found: %s", bdata(path));

    Request_set_action(conn->req, found);

    conn->req->target_host = host;
    conn->req->pattern = route->pattern;
    conn->req->prefix = route->prefix;

    return Connection_backend_event(found, conn);

error:
    return CLOSE;
}



int connection_msg_to_handler(Connection *conn)
{
    Handler *handler = Request_get_action(conn->req, handler);
    int rc = 0;
    int header_len = Request_header_length(conn->req);
    // body_len will include \0
    int body_len = Request_content_length(conn->req);

    check(handler, "JSON request doesn't match any handler: %s", 
            bdata(Request_path(conn->req)));

    if(pattern_match(IOBuf_start(conn->iob), header_len + body_len, bdata(&PING_PATTERN))) {
        Register_ping(IOBuf_fd(conn->iob));
    } else {
        check(body_len >= 0, "Parsing error, body length ended up being: %d", body_len);
        bstring payload = NULL;

        if(handler->protocol == HANDLER_PROTO_TNET) {
            payload = Request_to_tnetstring(conn->req, handler->send_ident,
                    IOBuf_fd(conn->iob), IOBuf_start(conn->iob) + header_len,
                    body_len - 1,conn, NULL);  // drop \0 on payloads
        } else if(handler->protocol == HANDLER_PROTO_JSON) {
            payload = Request_to_payload(conn->req, handler->send_ident,
                    IOBuf_fd(conn->iob), IOBuf_start(conn->iob) + header_len,
                    body_len - 1,conn, NULL);  // drop \0 on payloads
        } else {
            sentinel("Invalid protocol type: %d", handler->protocol);
        }

        debug("SENT: %s", bdata(payload));
        check(payload != NULL, "Failed to generate payload.");
        check(handler->send_socket != NULL, "Handler socket is NULL, tell Zed.");

        rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
        free(payload);
    
        check(rc == 0, "Failed to deliver to handler: %s", bdata(Request_path(conn->req)));

        conn->handler = handler;
    }

    // consumes \0 from body_len
    check(IOBuf_read_commit(conn->iob, header_len + body_len) != -1, "Final commit failed.");

    return REQ_SENT;

error:
    return CLOSE;
}

#define CERT_FINGERPRINT_SIZE 20

struct tagbstring PEER_CERT_SHA1_KEY = bsStatic("PEER_CERT_SHA1");

void Connection_fingerprint_from_cert(Connection *conn) 
{
    const mbedtls_x509_crt* _x509P  = mbedtls_ssl_get_peer_cert(&conn->iob->ssl);
    int i = 0;

    debug("Connection_fingerprint_from_cert: peer_cert: %016lX: tag=%d length=%ld",
            (unsigned long) _x509P,
            _x509P ? _x509P->raw.tag : -1,
            _x509P ? _x509P->raw.len : (unsigned)-1);

    if (_x509P != NULL && _x509P->raw.len > 0) {
        mbedtls_sha1_context	ctx;
        unsigned char sha1sum[CERT_FINGERPRINT_SIZE + 1] = {0};

        mbedtls_sha1_starts(&ctx);
        mbedtls_sha1_update(&ctx, _x509P->raw.p, _x509P->raw.len);
        mbedtls_sha1_finish(&ctx, sha1sum);

        bstring hex = bfromcstr("");
        for (i = 0; i < (int)sizeof(sha1sum); i++) {
            bformata(hex, "%02X", sha1sum[i]);
        }

        Request_set(conn->req, &PEER_CERT_SHA1_KEY, hex, 1);
    }
}

int Connection_send_to_handler(Connection *conn, Handler *handler, char *body, int content_len, hash_t *altheaders)
{
    int rc = 0;
    bstring payload = NULL;

    if(conn->iob->use_ssl)
	Connection_fingerprint_from_cert(conn);

    error_unless(handler->running, conn, 404,
            "Handler shutdown while trying to deliver: %s", bdata(Request_path(conn->req)));

    if(handler->protocol == HANDLER_PROTO_TNET) {
        payload = Request_to_tnetstring(conn->req, handler->send_ident,
                IOBuf_fd(conn->iob), body, content_len, conn, altheaders);
    } else if(handler->protocol == HANDLER_PROTO_JSON) {
        payload = Request_to_payload(conn->req, handler->send_ident,
                IOBuf_fd(conn->iob), body, content_len, conn, altheaders);
    } else {
        sentinel("Invalid protocol type: %d", handler->protocol);
    }

    check(payload, "Failed to create payload for request.");
    debug("HTTP TO HANDLER: %.*s", blength(payload) - content_len, bdata(payload));

    rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
    free(payload); payload = NULL;

    error_unless(rc != -1, conn, 502, "Failed to deliver to handler: %s", 
            bdata(Request_path(conn->req)));

    conn->handler = handler;
    return 0;

error:
    if(payload) free(payload);
    return -1;
}

static int Request_is_websocket(Request *req)
{
    bstring upgrade,connection;

    if(req->ws_flags != 0)
    {
        return req->ws_flags == 1;
    }

    if (Request_get(req, &WS_SEC_WS_KEY) !=NULL &&
        Request_get(req, &WS_SEC_WS_VER) !=NULL &&
        Request_get(req, &WS_HOST) !=NULL &&
        (upgrade = Request_get(req, &WS_UPGRADE)) != NULL &&
        (connection = Request_get(req, &WS_CONNECTION)) != NULL)
    {
        if (BSTR_ERR != binstrcaseless(connection,0,&WS_UPGRADE) &&
                biseqcaseless(upgrade,&WS_WEBSOCKET))
        {
            req->ws_flags =1;
            return 1;
        }
    }
    req->ws_flags =2;
    return 0;
}

static inline int is_websocket(Connection *conn)
{
    return Request_is_websocket(conn->req);
}

int connection_http_to_handler(Connection *conn)
{
    int content_len = Request_content_length(conn->req);
    int rc = 0;
    char *body = NULL;

    Handler *handler = Request_get_action(conn->req, handler);
    error_unless(handler, conn, 404, "No action for request: %s", bdata(Request_path(conn->req)));

    bstring expects = Request_get(conn->req, &HTTP_EXPECT);

    if (expects != NULL) {
        if (biseqcstr(expects, "100-continue")) {
            Response_send_status(conn, &HTTP_100);
        } else {
            Response_send_status(conn, &HTTP_417);
            log_info("Client requested unsupported expectation: %s.", bdata(expects));
            goto error;
        }
    }

    // we don't need the header anymore, so commit the buffer and deal with the body
    check(IOBuf_read_commit(conn->iob, Request_header_length(conn->req)) != -1, "Finaly commit failed streaming the connection to http handlers.");

    if(DOWNLOAD_FLOW_CONTROL && !conn->downloadCreditsInitialized) {
        conn->downloadCreditsInitialized = 1;
        Request_set(conn->req, &DOWNLOAD_CREDITS, bformat("%d", MAX_CREDITS), 1);
    }

    if(is_websocket(conn)) {
        bstring wsKey = Request_get(conn->req, &WS_SEC_WS_KEY);
        bstring response= websocket_challenge(wsKey);

        //Response_send_status(conn,response);
        bdestroy(conn->req->request_method);
        conn->req->request_method=bfromcstr("WEBSOCKET_HANDSHAKE");
        Connection_send_to_handler(conn, handler, bdata(response), blength(response), NULL);
        bdestroy(response);

        bdestroy(conn->req->request_method);
        conn->req->request_method=bfromcstr("WEBSOCKET");
        return REQ_SENT;
    }

    if(content_len == 0) {
        body = "";
        rc = Connection_send_to_handler(conn, handler, body, content_len, NULL);
        check_debug(rc == 0, "Failed to deliver to the handler.");
    } else if(content_len > MAX_CONTENT_LENGTH || content_len == -1) {
        if(Setting_get_int("upload.stream", 0)) {
            rc = Upload_stream(conn, handler, content_len);
        } else {
            rc = Upload_file(conn, handler, content_len);
        }
        check(rc == 0, "Failed to upload file.");
    } else {
        debug("READ ALL CALLED with content_len: %d, and MAX_CONTENT_LENGTH: %d", content_len, MAX_CONTENT_LENGTH);

        body = IOBuf_read_all(conn->iob, content_len, CLIENT_READ_RETRIES);
        check(body != NULL, "Client closed the connection during upload.");

        rc = Connection_send_to_handler(conn, handler, body, content_len, NULL);
        check_debug(rc == 0, "Failed to deliver to the handler.");
    }

    Log_request(conn, 200, content_len);

    return REQ_SENT;

error:
    return CLOSE;
}


int connection_http_to_directory(Connection *conn)
{
    Dir *dir = Request_get_action(conn->req, dir);

    int rc = Dir_serve_file(dir, conn->req, conn);
    check_debug(rc == 0, "Failed to serve file: %s", bdata(Request_path(conn->req)));

    check(IOBuf_read_commit(conn->iob,
            Request_header_length(conn->req) + Request_content_length(conn->req)) != -1, "Finaly commit failed sending from directory.");


    Log_request(conn, conn->req->status_code, conn->req->response_size);

    if(conn->close) {
        return CLOSE;
    } else {
        return RESP_SENT;
    }

error:
    return CLOSE;
}




int connection_http_to_proxy(Connection *conn)
{
    Proxy *proxy = Request_get_action(conn->req, proxy);
    check(proxy != NULL, "Should have a proxy backend.");

    debug("CONNECT TO: %s:%d", bdata(proxy->server), proxy->port);

    int proxy_fd = netdial(1, bdata(proxy->server), proxy->port);
    check(proxy_fd != -1, "Failed to connect to proxy backend %s:%d",
            bdata(proxy->server), proxy->port);

    if(!conn->proxy_iob) {
        conn->proxy_iob = IOBuf_create(BUFFER_SIZE, proxy_fd, IOBUF_SOCKET);
        check_mem(conn->proxy_iob);
    }

    if(!conn->client) {
        conn->client = calloc(sizeof(httpclient_parser), 1);
        check_mem(conn->client);
    }

    return CONNECT;

error:
    return FAILED;
}

int connection_proxy_deliver(Connection *conn)
{
    int rc = 0;
    int total_len = Request_header_length(conn->req) + Request_content_length(conn->req);

    char *buf =NULL;


    if (conn->req->new_header) {
        log_info("In Proxy.");
        IOBuf_read_all(conn->iob,Request_header_length(conn->req),CLIENT_READ_RETRIES);

        buf = IOBuf_read_all(conn->iob, Request_content_length(conn->req),
                CLIENT_READ_RETRIES);
        check(buf != NULL, "Failed to read from the client socket to proxy.");

        rc = IOBuf_send(conn->proxy_iob, bdata(conn->req->new_header),
                blength(conn->req->new_header));
        check(rc > 0, "Failed to send to proxy.");

        if(Request_content_length(conn->req) > 0) {
            rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob),
                    Request_content_length(conn->req));
            check(rc > 0, "Failed to send to proxy.");
        }
    } else {

        buf = IOBuf_read_all(conn->iob, total_len, CLIENT_READ_RETRIES);
        check(buf != NULL, "Failed to read from the client socket to proxy.");

        rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob), total_len);
        check(rc > 0, "Failed to send to proxy.");
    }

    return REQ_SENT;

error:
    return REMOTE_CLOSE;
}

int connection_proxy_reply_parse(Connection *conn)
{
    int rc = 0;
    int total = 0;
    Proxy *proxy = Request_get_action(conn->req, proxy);
    check(proxy != NULL, "Proxy is NULL in reply_parse?")
    httpclient_parser *client = conn->client;

    rc = Proxy_read_and_parse(conn);
    check(rc != -1, "Failed to read from proxy server: %s:%d", 
            bdata(proxy->server), proxy->port);

    if(client->chunked) {
        // send the http header we have so far
        rc = IOBuf_stream(conn->proxy_iob, conn->iob, client->body_start);
        check_debug(rc != -1, "Failed streaming header to client.");

        // then just stream out the chunks we've got, as long as 
        // Proxy_stream_chunks return 0 it means we've got more to send
        do {
            rc = Proxy_stream_chunks(conn);
            check(rc != -1, "Failed to stream chunked encoding to client.");
        } while(rc == 0);

    } else if(client->content_len >= 0) {
        total = client->body_start + client->content_len;
        rc = IOBuf_stream(conn->proxy_iob, conn->iob, total);
        check(rc != -1, "Failed streaming non-chunked response.");
    } else if(client->status == 204 || client->status == 304) {
        //  According to the RFC, these MUST NOT incude a body.
        //  Proxy might keep the connection open but not send content-length.
        rc = IOBuf_stream(conn->proxy_iob, conn->iob, client->body_start);
        check(rc != -1, "Failed streaming non-chunked response.");
    } else if(client->close || client->content_len == -1) {
        debug("Response requested a read until close.");
        check(Proxy_stream_to_close(conn) != -1, "Failed streaming to client.");
    } else {
        sentinel("Should not reach this code, Tell Zed.");
    }

    Log_request(conn, client->status, client->content_len);

    if(client->close) {
        return REMOTE_CLOSE;
    } else {
        return REQ_RECV;
    }

error:
    return FAILED;
}


int connection_proxy_req_parse(Connection *conn)
{
    Route *route = NULL;

    int rc = 0;
    Host *target_host = conn->req->target_host;
    Backend *req_action = conn->req->action;

    check_debug(!IOBuf_closed(conn->iob), "Client closed, goodbye.");

    rc = Connection_read_header(conn, conn->req);

    check_debug(rc > 0, "Failed to read another header.");
    error_unless(Request_is_http(conn->req), conn, 400,
            "Someone tried to change the protocol on us from HTTP.");

    // add the x-forwarded-for header
    Request_set(conn->req, &HTTP_X_FORWARDED_FOR, bfromcstr(conn->remote), 1);

    Backend *found = Host_match_backend(target_host, Request_path(conn->req), &route);
    error_unless(found, conn, 404, 
            "Handler not found: %s", bdata(Request_path(conn->req)));

    // break out of PROXY if the actions don't match
    if(found != req_action) {
        Request_set_action(conn->req, found);
        conn->req->pattern = route->pattern;
        conn->req->prefix = route->prefix;
        return Connection_backend_event(found, conn);
    } else {
        return HTTP_REQ;
    }

    error_response(conn, 500, "Invalid code branch, tell Zed.");
error:
    return REMOTE_CLOSE;
}



int connection_proxy_failed(Connection *conn)
{
    Response_send_status(conn, &HTTP_502);

    return CLOSE;
}


int connection_proxy_close(Connection *conn)
{
    IOBuf_destroy(conn->proxy_iob);
    conn->proxy_iob = NULL;

    return CLOSE;
}

static inline int close_or_error(Connection *conn, int next)
{

    IOBuf_destroy(conn->proxy_iob);
    conn->proxy_iob = NULL;

    if (IOBuf_fd(conn->iob) >= 0)
        check_debug(Register_disconnect(IOBuf_fd(conn->iob)) != -1,
                "Register disconnect didn't work for %d", IOBuf_fd(conn->iob));

error:
    // fallthrough on purpose
    return next;
}


int connection_close(Connection *conn)
{
    return close_or_error(conn, 0);
}



int connection_error(Connection *conn)
{
    return close_or_error(conn, CLOSE);
}

int connection_identify_request(Connection *conn)
{
    int next = CLOSE;

    if(Request_is_xml(conn->req)) {
        if(biseq(Request_path(conn->req), &POLICY_XML_REQUEST)) {
            debug("XML POLICY CONNECTION: %s", bdata(Request_path(conn->req)));
            conn->type = CONN_TYPE_SOCKET;
            taskname("XML");
            next = SOCKET_REQ;
        } else {
            debug("XML MESSAGE");
            conn->type = CONN_TYPE_MSG;
            taskname("MSG");
            next = MSG_REQ;
        }
    } else if(Request_is_json(conn->req)) {
        debug("JSON SOCKET MESSAGE");
        conn->type = CONN_TYPE_MSG;
        taskname("MSG");
        next = MSG_REQ;
    } else if(Request_is_websocket(conn->req)) {
        debug("WEBSOCKET MESSAGE");
        conn->type = CONN_TYPE_SOCKET;
        taskname("WS");
        next = WS_REQ;
    } else if(Request_is_http(conn->req)) {
        debug("HTTP MESSAGE");
        conn->type = CONN_TYPE_HTTP;
        taskname("HTTP");
        next = HTTP_REQ;
    } else {
        error_response(conn, 500, "Invalid code branch, tell Zed.");
    }

    return next;

error:
    return CLOSE;

}


int connection_parse(Connection *conn)
{
    if(Connection_read_header(conn, conn->req) > 0) {
        if(!Setting_get_int("no_clobber_xff", 0)) {
            // add the x-forwarded-for header
            Request_set(conn->req, &HTTP_X_FORWARDED_FOR, bfromcstr(conn->remote), 1);
        }
        return REQ_RECV;
    } else {
        return CLOSE;
    }
}



int Connection_read_wspacket(Connection *conn)
{
    bstring payload = NULL;
    int inprogFlags = 0;
    char key[4];
    int isControl;
    int flags;
    uint8_t *dataU;
    char *data;
    int avail;
    int64_t packet_length;
    int smaller_packet_length;
    int header_length;
    int i;
    int data_length;
    int tries;
    int rc;
    int fin;

again:
    dataU = NULL;
    data = IOBuf_start(conn->iob);
    avail = IOBuf_avail(conn->iob);
    packet_length = -1;

    for(tries = 0; packet_length == -1 && tries < 8*CLIENT_READ_RETRIES; tries++) {
        if(avail > 0) {
            packet_length = Websocket_packet_length((uint8_t *)data, avail);
        }

        if(packet_length == -1) {
            data = IOBuf_read_some(conn->iob, &avail);
            check_debug(!IOBuf_closed(conn->iob), "Client closed during read.");
        }
    }
    check(packet_length > 0,"Error receiving websocket packet header.");

    check_debug(packet_length <= INT_MAX,"Websocket packet longer than MAXINT.");
    /* TODO properly terminate WS connection */

    smaller_packet_length = (int)packet_length;

    /* TODO check for maximum length */

    header_length=Websocket_header_length((uint8_t *) data, avail);
    data_length=smaller_packet_length-header_length;
    dataU = (uint8_t *)IOBuf_read_all(conn->iob,header_length,8*CLIENT_READ_RETRIES);
    check(dataU != NULL, "Client closed the connection during websocket packet.");
    memcpy(key,dataU+header_length-4,4);

    flags=dataU[0];
    if (payload==NULL) {
        inprogFlags=flags;
    }


    fin = (WS_fin(dataU));
    isControl=(WS_is_control(dataU));

    {
        const char *error=WS_validate_packet(dataU,payload!=NULL);
        check(error==NULL,"%s",error);
    }

    dataU = (uint8_t *)IOBuf_read_all(conn->iob,data_length, 8*CLIENT_READ_RETRIES);
    check(dataU != NULL, "Client closed the connection during websocket packet.");

    for(i=0;i<data_length;++i) {
        dataU[i]^=key[i%4];
    }

    if(isControl) /* Control frames get sent right-away */
    {
        Request_set(conn->req,&WS_FLAGS,bformat("0x%X",flags|0x80),1);
        rc = Connection_send_to_handler(conn, conn->handler, (void *)dataU,data_length, NULL);
        check_debug(rc == 0, "Failed to deliver to the handler.");
    }
    else {
        if(fin) {
            Request_set(conn->req,&WS_FLAGS,bformat("0x%X",inprogFlags|0x80),1);
        }
        if (payload == NULL) {
            if (fin) {
                rc = Connection_send_to_handler(conn, conn->handler, (void *)dataU,data_length, NULL);
                check_debug(rc == 0, "Failed to deliver to the handler.");
            }
            else {
                payload = blk2bstr(dataU,data_length);
                check(payload != NULL,"Allocation failed");
            }
        } else {
            check(BSTR_OK == bcatblk(payload,dataU,data_length), "Concatenation failed");
            if (fin) {
                rc = Connection_send_to_handler(conn, conn->handler, bdata(payload),blength(payload), NULL);
                check_debug(rc == 0, "Failed to deliver to the handler.");
                bdestroy(payload);
                payload=NULL;
            }
        }
    }
    if (payload != NULL) {
        goto again;
    }


    return packet_length;

error:
    bdestroy(payload);
    return -1;

}

int connection_websocket_established(Connection *conn)
{
    if(Connection_read_wspacket(conn) > 0) {
        return REQ_SENT;
    } else {
        return CLOSE;
    }

    debug("WS Established");
    return 0;
}

StateActions CONN_ACTIONS = {
    .error = connection_error,
    .close = connection_close,
    .parse = connection_parse,
    .register_request = connection_identify_request,
    .identify_request = connection_identify_request,
    .route_request = connection_route_request,
    .send_socket_response = connection_send_socket_response,
    .msg_to_handler = connection_msg_to_handler,
    .http_to_handler = connection_http_to_handler,
    .http_to_proxy = connection_http_to_proxy,
    .http_to_directory = connection_http_to_directory,
    .proxy_deliver = connection_proxy_deliver,
    .proxy_failed = connection_proxy_failed,
    .proxy_reply_parse = connection_proxy_reply_parse,
    .proxy_req_parse = connection_proxy_req_parse,
    .proxy_close = connection_proxy_close,
    .websocket_established = connection_websocket_established
};


void Connection_deliver_task_kill(Connection *conn)
{
    if(conn && conn->deliverTaskStatus==DT_RUNNING) {
        debug("Killing task for connection %p",conn);
        int sleeptime=10;
        while(!list_isempty(conn->deliverQueue))
        {
            Deliver_message msg={NULL,NULL};
            Connection_deliver_dequeue(conn,&msg);
            tns_value_destroy(msg.data);
        }
        Connection_deliver_enqueue(conn,NULL,NULL);
        conn->deliverTaskStatus=DT_DYING;
        while(conn->deliverTaskStatus != DT_DEAD) {
            taskdelay(sleeptime);
            if(sleeptime<10000) {
                sleeptime<<=1;
            }
            else {
                log_warn("Connection %p is not dying, contact jasom\n",conn);
                //*((int *)0)=1;
            }
        }
        debug("Deliver Task Killed %p",conn);
    }
}

void Connection_destroy(Connection *conn)
{
    if(conn) {
        Connection_deliver_task_kill(conn);

        if(conn->deliverQueue != NULL) {
            list_process(conn->deliverQueue, NULL, free_lnode);
            list_destroy_nodes(conn->deliverQueue);
            list_destroy(conn->deliverQueue);
            conn->deliverQueue = NULL;
        }

        Request_destroy(conn->req);
        conn->req = NULL;

        if(conn->client) free(conn->client);
        IOBuf_destroy(conn->iob);
        IOBuf_destroy(conn->proxy_iob);
        free(conn);
    }
}

/* Comparision function for storing in tree when we don't care about lexicographic ordering
 * O(1) when lengths are different
 */
static int blkcmp(const void *x, const void *y)
{
    const_bstring a = x;
    const_bstring b = y;
    if(blengthe(a,0) < blengthe(b,0)) {
        return -1;
    }
    if(blengthe(a,0) > blengthe(b,0)) {
        return 1;
    }

    return memcmp(a->data, b->data, blengthe(b,0));
}

typedef struct sni_information {
    mbedtls_x509_crt own_cert;
    mbedtls_pk_context pk_key;
} sni_information;

static int sni_cache_lookup(Connection *conn, bstring hostname)
{
    dnode_t *match = NULL;
    sni_information *matchi = NULL;
    if (NULL == sni_cache) {
        debug("Creating sni cache");
        sni_cache = malloc(sizeof(*sni_cache));
        check(sni_cache != NULL, "Allocation failed");
        dict_init(sni_cache, (unsigned long)-1, blkcmp);
        return 1;
    }

    match = dict_lookup(sni_cache, hostname);

    check_debug(match != NULL, "Unable to find match in cache");

    matchi = match->dict_data;

    conn->own_cert = matchi->own_cert;
    conn->pk_key = matchi->pk_key;

    return 0;

error:
    return 1;
}

static int sni_cache_add(Connection *conn, bstring hostname)
{
    bstring newkey = bstrcpy(hostname);
    sni_information* newval = malloc(sizeof(sni_information));

    check(newkey != NULL && newval != NULL, "Allocation failed");

    newval->own_cert = conn->own_cert;
    newval->pk_key = conn->pk_key;

    check(dict_alloc_insert(sni_cache, newkey, newval), "Allocation failed");

    return 0;

error:
    bdestroy(newkey);
    if(newval != NULL) free(newval);
    return 1;
}

static void free_sni_info(dict_t *dict, dnode_t * node, void *context)
{
    (void)(dict);    /* Unused */
    (void)(context); /* Unused */
    bdestroy((bstring)node->dict_key);
    free(node->dict_data);
}

void Connection_flush_sni_cache()
{
    if(sni_cache != NULL) {
        dict_process(sni_cache, NULL, free_sni_info);
        dict_free_nodes(sni_cache);
    }
}


static int connection_sni_cb(void *p_conn, mbedtls_ssl_context *ssl, const unsigned char *chostname, size_t chostname_len)
{
    Connection *conn = (Connection *) p_conn;
    int i;
    int rc = 0;
    bstring hostname = NULL;
    bstring certpath = NULL;
    bstring keypath = NULL;
    bstring tryhostpattern = NULL;
    bstring tmpstr;

    hostname = blk2bstr((const char *)chostname, chostname_len);
    check(hostname != NULL, "Allocation failed");

    if (!sni_cache_lookup(conn,  hostname)) goto success;
        
    // input is utf-8. we'll forbid ascii-control chars and '/', for safe
    //   filesystem usage. multibyte characters always have the high bit set
    //   on each byte, so there's no worry of them getting caught.
    for(i = 0; i < blength(hostname); ++i) {
        unsigned char c = bdata(hostname)[i];
        if(c < 0x20 || c == '/') {
            log_warn("SNI: invalid hostname provided");
            return -1;
        } else if(c <= 0x7f) {
            // FIXME: it's wrong to use tolower() on a utf8 string, but it
            //   will probably work well enough for most people
            bdata(hostname)[i] = (unsigned char)tolower(c);
        }
    }

    bstring certdir = Setting_get_str("certdir", NULL);
    check(certdir != NULL, "to use ssl, you must specify a certdir");

    // try to find a file named after the exact host, then try with a wildcard pattern at the
    //   same subdomain level. the file format uses underscores instead of asterisks. so, a
    //   domain of www.example.com will attempt to be matched against a file named
    //   www.example.com.crt and _.example.com.crt. wildcards at other levels are not supported.

    tryhostpattern = bstrcpy(hostname);
    certpath = bformat("%s%s.crt", bdata(certdir), bdata(tryhostpattern));
    check_mem(certpath);

    rc = mbedtls_x509_crt_parse_file(&conn->own_cert, bdata(certpath));
    if(rc != 0) {
        i = bstrchr(tryhostpattern, '.');
        check(i != BSTR_ERR, "Failed to find cert for %s", bdata(hostname));

        tmpstr = bmidstr(tryhostpattern, i, blength(tryhostpattern) - i);
        bdestroy(tryhostpattern);
        tryhostpattern = bfromcstr("_");
        bconcat(tryhostpattern, tmpstr);
        bdestroy(tmpstr);

        certpath = bformat("%s%s.crt", bdata(certdir), bdata(tryhostpattern));
        check_mem(certpath);

        rc = mbedtls_x509_crt_parse_file(&conn->own_cert, bdata(certpath));
        check(rc == 0, "Failed to find cert for %s", bdata(hostname));
    }

    keypath = bformat("%s%s.key", bdata(certdir), bdata(tryhostpattern));
    check_mem(keypath);

    rc = mbedtls_pk_parse_keyfile(&conn->pk_key, bdata(keypath), NULL);
    check(rc == 0, "Failed to load key from %s", bdata(keypath));

    check(0 == sni_cache_add(conn, hostname), "Error adding entry to SNI cache");

success:

    bdestroy(hostname);
    bdestroy(tryhostpattern);
    bdestroy(certpath);
    bdestroy(keypath);

    conn->use_sni = 1;

    mbedtls_ssl_set_hs_own_cert(ssl, &conn->own_cert, &conn->pk_key);

    return 0;

error:
    // it should be safe to call these on zeroed-out objects
    mbedtls_x509_crt_free(&conn->own_cert);
    mbedtls_pk_free(&conn->pk_key);

    bdestroy(hostname);
    if(tryhostpattern != NULL) bdestroy(tryhostpattern);
    if(certpath != NULL) bdestroy(certpath);
    if(keypath != NULL) bdestroy(keypath);

    // don't return error here. this way the ssl handshake continues and the
    //   the default cert will get used instead
    return 0;
}


Connection *Connection_create(Server *srv, int fd, int rport,
                              const char *remote)
{
    int rc = 0;
    Connection *conn = calloc(sizeof(Connection),1);
    check_mem(conn);

    conn->req = Request_create();
    conn->proxy_iob = NULL;
    conn->use_sni = 0;
    conn->rport = rport;
    conn->client = NULL;
    conn->close = 0;
    conn->type = 0;
    conn->filter_state = NULL;

    strncpy(conn->remote, remote, IPADDR_SIZE);
    conn->remote[IPADDR_SIZE] = '\0';

    conn->handler = NULL;

    conn->deliverQueue = list_create(LISTCOUNT_T_MAX);
    check_mem(conn->deliverQueue);

    conn->downloadCreditsInitialized = 0;
    conn->sendCredits = 0;

    check_mem(conn->req);

    Request_set_relaxed(conn->req, RELAXED_PARSING);

    if(srv != NULL && srv->use_ssl) {
        conn->iob = IOBuf_create_ssl(BUFFER_SIZE, fd, srv->rng_func, srv->rng_ctx);
        check(conn->iob != NULL, "Failed to create the SSL IOBuf.");

        // set default cert
        mbedtls_ssl_conf_own_cert(&conn->iob->sslconf, &srv->own_cert, &srv->pk_key);

        // set the ca_chain if it was specified in settings
        if ( srv->ca_chain.version != -1 ) {
            mbedtls_ssl_conf_ca_chain(&conn->iob->sslconf, &srv->ca_chain, NULL );
        }

        // setup callback for SNI. if the client does not use this feature,
        //   then this callback is never invoked and the above default cert
        //   will be used
        mbedtls_ssl_conf_sni(&conn->iob->sslconf, connection_sni_cb, conn);

        mbedtls_ssl_conf_dh_param(&conn->iob->sslconf, srv->dhm_P, srv->dhm_G);
        mbedtls_ssl_conf_ciphersuites(&conn->iob->sslconf, srv->ciphers);

        // now that the ssl configuration is set, we can init
        rc = IOBuf_ssl_init(conn->iob);
        check(rc == 0, "Failed to init SSL.");
    } else {
        conn->iob = IOBuf_create(BUFFER_SIZE, fd, IOBUF_SOCKET);
    }

    return conn;

error:
    Connection_destroy(conn);
    return NULL;
}


int Connection_accept(Connection *conn)
{
    check(Register_connect(IOBuf_fd(conn->iob), (void*)conn) != -1,
            "Failed to register connection.");

    check(taskcreate(Connection_task, conn, CONNECTION_STACK) != -1,
            "Failed to create connection task.");
    
    check(taskcreate(Connection_deliver_task, conn, CONNECTION_STACK) != -1,
            "Failed to create connection task.");
    conn->deliverTaskStatus=DT_RUNNING;
    return 0;
error:
    IOBuf_register_disconnect(conn->iob);
    return -1;
}



void Connection_task(void *v)
{
    Connection *conn = (Connection *)v;
    int i = 0;
    int next = OPEN;

    State_init(&conn->state, &CONN_ACTIONS);

    while(1) {
        if(Filter_activated()) {
            next = Filter_run(next, conn);
            check(next >= CLOSE && next < EVENT_END,
                    "!!! Invalid next event[%d]: %d from filter!", i, next);
        }

        if(next == CLOSE) break;

        next = State_exec(&conn->state, next, (void *)conn);

        check(next >= CLOSE && next < EVENT_END,
                "!!! Invalid next event[%d]: %d, Tell ZED!", i, next);

        if(conn->iob && !conn->iob->closed) {
            Register_ping(IOBuf_fd(conn->iob));
        }

        i++;
    }

error: // fallthrough
    if(conn->handler) {
        Connection_handler_notify_leave(conn);
    }

    State_exec(&conn->state, CLOSE, (void *)conn);
    Connection_destroy(conn);
    taskexit(0);
}

int Connection_deliver_raw_internal(Connection *conn, tns_value_t *data)
{
    bstring buf;
    int rc;
    int fd;
    uint32_t id;

    fd = IOBuf_fd(conn->iob);
    id = Register_id_for_fd(fd);
    check_debug(id != UINT32_MAX, "Asked to write to an fd that doesn't exist: %d", fd);

    check(conn->handler != NULL, "handler not set when delivering");

    check(data->type==tns_tag_string, "deliver_raw_internal expected a string.");
    buf=data->value.string;

    rc = IOBuf_send_all(conn->iob, bdata(buf), blength(buf));
    check_debug(rc==blength(buf), "Failed to send all of the data: %d of %d", rc, blength(buf));

    if(DOWNLOAD_FLOW_CONTROL) {
        Handler_notify_credits(conn->handler, id, rc);
    }

    return 0;

error:
    return -1;
}

int Connection_deliver_raw(Connection *conn, bstring buf)
{
    tns_value_t *val=NULL;
    if(buf != NULL) {
        val=malloc(sizeof(tns_value_t));
        check_mem(val);
        val->type=tns_tag_string;
        val->value.string=bstrcpy(buf);
        check_debug(0== Connection_deliver_enqueue(conn, Connection_deliver_raw_internal,val),
            "Failed to write raw message to con %d", IOBuf_fd(conn->iob));
    } else {
        conn->closing = 1;
        Connection_deliver_enqueue(conn, NULL, NULL);
    }

    return 0;
error:
    tns_value_destroy(val);
    return -1;
}

int Connection_deliver(Connection *conn, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bBase64Encode(buf);
    check(b64_buf != NULL, "Failed to base64 encode data.");
    check(conn->iob != NULL, "There's no IOBuffer to send to, Tell Zed.");
    /* Yep there's an extraneous copy, but the deliver raw function is better
     * tested so for maintainability we will use it */
    rc = Connection_deliver_raw(conn,b64_buf);
    check_debug(rc == 0, "Failed to write message to conn %d", IOBuf_fd(conn->iob));
    bdestroy(b64_buf);

    return 0;

error:
    bdestroy(b64_buf);
    return -1;
}


void Connection_deliver_task(void *v)
{
    Connection *conn=v;
    Deliver_message msg={NULL,NULL};
    while(1) {
        Connection_deliver_dequeue(conn, &msg);
        check_debug(msg.deliver,"Received NULL msg on FD %d, exiting deliver task",IOBuf_fd(conn->iob));
        check(-1 != msg.deliver(conn,msg.data),"Error delivering to MSG listener on FD %d, closing them.", IOBuf_fd(conn->iob));
        tns_value_destroy(msg.data);
        msg.data=NULL;
    }
error:
    conn->handler = NULL; // we're shutting down, don't notify anything else
    conn->deliverTaskStatus=DT_DYING;
    tns_value_destroy(msg.data);
    while(!list_isempty(conn->deliverQueue)) {
        Connection_deliver_dequeue(conn, &msg);
        tns_value_destroy(msg.data);
    }

    IOBuf_shutdown(conn->iob);

    debug("Deliver Task Shut Down\n");
    conn->deliverTaskStatus=DT_DEAD;
    taskexit(0);
}

static inline void check_should_close(Connection *conn, Request *req)
{
    if(req->version && biseqcstr(req->version, "HTTP/1.0")) {
        debug("HTTP 1.0 request coming in from %s", conn->remote);
        conn->close = 1;
    } else {
        bstring conn_close = Request_get(req, &HTTP_CONNECTION);

        if(conn_close && biseqcstrcaseless(conn_close, "close")) {
            conn->close = 1;
        } else {
            conn->close = 0;
        }
    }
}


int Connection_read_header(Connection *conn, Request *req)
{
    char *data = IOBuf_start(conn->iob);
    int avail = IOBuf_avail(conn->iob);
    int rc = 0;
    size_t nparsed = 0;
    int tries = 0;

    Request_start(req);

    for(tries = 0; rc == 0 && tries < CLIENT_READ_RETRIES; tries++) {
        if(avail > 0) {
            rc = Request_parse(req, data, avail, &nparsed);
        }

        if(rc == 0) {
            data = IOBuf_read_some(conn->iob, &avail);
            check_debug(!IOBuf_closed(conn->iob), "Client closed during read.");
        }
    }

    error_unless(tries < CLIENT_READ_RETRIES, conn, 
            400, "Too many small packet read attempts.");
    error_unless(rc == 1, conn, 400, "Error parsing request.");

    check_should_close(conn, conn->req);

    return nparsed;

error:
    return -1;

}


void Connection_init()
{
    MAX_CONTENT_LENGTH = Setting_get_int("limits.content_length", 20 * 1024);
    BUFFER_SIZE = Setting_get_int("limits.buffer_size", 4 * 1024);
    CONNECTION_STACK = Setting_get_int("limits.connection_stack_size", 32 * 1024);
    CLIENT_READ_RETRIES = Setting_get_int("limits.client_read_retries", 5);


    log_info("MAX limits.content_length=%d, limits.buffer_size=%d, limits.connection_stack_size=%d, limits.client_read_retries=%d",
            MAX_CONTENT_LENGTH, BUFFER_SIZE, CONNECTION_STACK,
            CLIENT_READ_RETRIES);

    PROXY_READ_RETRIES = Setting_get_int("limits.proxy_read_retries", 100);
    PROXY_READ_RETRY_WARN = Setting_get_int("limits.proxy_read_retry_warn", 10);

    log_info("MAX limits.proxy_read_retries=%d, limits.proxy_read_retry_warn=%d",
            PROXY_READ_RETRIES, PROXY_READ_RETRY_WARN);

    IO_SSL_VERIFY_METHOD = Setting_get_int("ssl.verify_optional", 0) ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_NONE;
    IO_SSL_VERIFY_METHOD = Setting_get_int("ssl.verify_required", 0) ? MBEDTLS_SSL_VERIFY_REQUIRED : IO_SSL_VERIFY_METHOD;

    RELAXED_PARSING = Setting_get_int("request.relaxed", 0);
    DOWNLOAD_FLOW_CONTROL = Setting_get_int("download.flow_control", 0);
}


