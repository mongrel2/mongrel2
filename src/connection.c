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
#include "headers.h"

struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":\\s*\"ping\"}");

struct tagbstring POLICY_XML_REQUEST = bsStatic("<policy-file-request");

int MAX_CONTENT_LENGTH = 20 * 1024;
int BUFFER_SIZE = 4 * 1024;
int CONNECTION_STACK = 32 * 1024;
int CLIENT_READ_RETRIES = 5;

int MAX_WS_LENGTH = 256 * 1024;


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


static inline int Connection_deliver_enqueue(Connection *conn, bstring b)
{
    check_debug(conn->deliverPost-conn->deliverAck < DELIVER_OUTSTANDING_MSGS, "Too many outstanding messages") ;
    conn->deliverRing[conn->deliverPost++%DELIVER_OUTSTANDING_MSGS]=b;
    taskwakeup(&conn->deliverRendez);
    return 0;

error:
    return -1;
}

static inline bstring Connection_deliver_dequeue(Connection *conn)
{
    while(1) {
        if(conn->deliverPost-conn->deliverAck) {
            return conn->deliverRing[conn->deliverAck++%DELIVER_OUTSTANDING_MSGS];
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
                    body_len - 1);  // drop \0 on payloads
        } else if(handler->protocol == HANDLER_PROTO_JSON) {
            payload = Request_to_payload(conn->req, handler->send_ident,
                    IOBuf_fd(conn->iob), IOBuf_start(conn->iob) + header_len,
                    body_len - 1);  // drop \0 on payloads
        } else {
            sentinel("Invalid protocol type: %d", handler->protocol);
        }

        debug("SENT: %s", bdata(payload));
        check(payload != NULL, "Failed to generate payload.");
        check(handler->send_socket != NULL, "Handler socket is NULL, tell Zed.");

        rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
        free(payload);
    
        check(rc == 0, "Failed to deliver to handler: %s", bdata(Request_path(conn->req)));
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
    x509_cert* _x509P  = conn->iob->ssl.peer_cert;
    int i = 0;

    debug("Connection_send_to_handler: peer_cert: %016lX: tag=%d length=%ld",
            (unsigned long) _x509P,
            _x509P ? _x509P->raw.tag : -1,
            _x509P ? _x509P->raw.len : -1);

    if (_x509P != NULL && _x509P->raw.len > 0) {
        sha1_context	ctx;
        unsigned char sha1sum[CERT_FINGERPRINT_SIZE + 1] = {0};

        sha1_starts(&ctx);
        sha1_update(&ctx, _x509P->raw.p, _x509P->raw.len);
        sha1_finish(&ctx, sha1sum);

        bstring hex = bfromcstr("");
        for (i = 0; i < (int)sizeof(sha1sum); i++) {
            bformata(hex, "%02X", sha1sum[i]);
        }

        Request_set(conn->req, &PEER_CERT_SHA1_KEY, hex, 1);
    }
}

int Connection_send_to_handler(Connection *conn, Handler *handler, char *body, int content_len)
{
    int rc = 0;
    bstring payload = NULL;

    if(conn->iob->use_ssl)
	Connection_fingerprint_from_cert(conn);

    error_unless(handler->running, conn, 404,
            "Handler shutdown while trying to deliver: %s", bdata(Request_path(conn->req)));

    if(handler->protocol == HANDLER_PROTO_TNET) {
        payload = Request_to_tnetstring(conn->req, handler->send_ident, 
                IOBuf_fd(conn->iob), body, content_len);
    } else if(handler->protocol == HANDLER_PROTO_JSON) {
        payload = Request_to_payload(conn->req, handler->send_ident, 
                IOBuf_fd(conn->iob), body, content_len);
    } else {
        sentinel("Invalid protocol type: %d", handler->protocol);
    }

    check(payload, "Failed to create payload for request.");
    debug("HTTP TO HANDLER: %.*s", blength(payload) - content_len, bdata(payload));

    rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
    free(payload); payload = NULL;

    error_unless(rc != -1, conn, 502, "Failed to deliver to handler: %s", 
            bdata(Request_path(conn->req)));

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
                !bstrcmp(upgrade,&WS_WEBSOCKET))
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

    if(is_websocket(conn)) {
        bstring wsKey = Request_get(conn->req, &WS_SEC_WS_KEY);
        bstring response= websocket_challenge(wsKey);
        conn->handler = handler;

        //Response_send_status(conn,response);
        bdestroy(conn->req->request_method);
        conn->req->request_method=bfromcstr("WEBSOCKET_HANDSHAKE");
        Connection_send_to_handler(conn, handler, bdata(response), blength(response));
        bdestroy(response);

        bdestroy(conn->req->request_method);
        conn->req->request_method=bfromcstr("WEBSOCKET");
        return REQ_SENT;
    }

    if(content_len == 0) {
        body = "";
        rc = Connection_send_to_handler(conn, handler, body, content_len);
        check_debug(rc == 0, "Failed to deliver to the handler.");
    } else if(content_len > MAX_CONTENT_LENGTH) {
        rc = Upload_file(conn, handler, content_len);
        check(rc == 0, "Failed to upload file.");
    } else {
        debug("READ ALL CALLED with content_len: %d, and MAX_CONTENT_LENGTH: %d", content_len, MAX_CONTENT_LENGTH);

        body = IOBuf_read_all(conn->iob, content_len, CLIENT_READ_RETRIES);
        check(body != NULL, "Client closed the connection during upload.");

        rc = Connection_send_to_handler(conn, handler, body, content_len);
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

    char *buf = IOBuf_read_all(conn->iob, total_len, CLIENT_READ_RETRIES);
    check(buf != NULL, "Failed to read from the client socket to proxy.");

    rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob), total_len);
    check(rc > 0, "Failed to send to proxy.");

    return REQ_SENT;

error:
    return REMOTE_CLOSE;
}



int connection_proxy_reply_parse(Connection *conn)
{
    int rc = 0;
    int total = 0;
    Proxy *proxy = Request_get_action(conn->req, proxy);
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
    int rc = 0;
    Host *target_host = conn->req->target_host;
    Backend *req_action = conn->req->action;

    check_debug(!IOBuf_closed(conn->iob), "Client closed, goodbye.");

    rc = Connection_read_header(conn, conn->req);

    check_debug(rc > 0, "Failed to read another header.");
    error_unless(Request_is_http(conn->req), conn, 400,
            "Someone tried to change the protocol on us from HTTP.");

    Backend *found = Host_match_backend(target_host, Request_path(conn->req), NULL);
    error_unless(found, conn, 404, 
            "Handler not found: %s", bdata(Request_path(conn->req)));

    // break out of PROXY if the actions don't match
    if(found != req_action) {
        Request_set_action(conn->req, found);
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
        return REQ_RECV;
    } else {
        return CLOSE;
    }
}



int Connection_read_wspacket(Connection *conn)
{
    bstring payload=NULL;

    uint8_t *dataU=NULL;
    char *data = IOBuf_start(conn->iob);
    int avail = IOBuf_avail(conn->iob);
    int64_t packet_length=-1;
    int smaller_packet_length;
    int header_length;
    char key[4];
    int i;
    int data_length;
    int tries = 0;
    int rc=0;
    int fin;
    int inprogFlags=0;
    int isControl;
    int flags;

again:
    dataU=NULL;
    data = IOBuf_start(conn->iob);
    avail = IOBuf_avail(conn->iob);
    packet_length=-1;
    smaller_packet_length=0;
    header_length=0;
    i=0;
    data_length=0;
    tries = 0;
    rc=0;
    fin=0;

    for(tries = 0; packet_length == -1 && tries < 8*CLIENT_READ_RETRIES; tries++) {
        if(avail > 0) {
            packet_length = Websocket_packet_length((uint8_t *)data, avail);
        }

        if(packet_length == -1) {
            data = IOBuf_read_some(conn->iob, &avail);
            check_debug(!IOBuf_closed(conn->iob), "Client closed during read.");
        }
    }
    check(packet_length > 0,"Error receiving websocket packet header.")

    check_debug(packet_length <= INT_MAX,"Websocket packet longer than MAXINT.");
    /* TODO properly terminate WS connection */

    smaller_packet_length = (int)packet_length;
    
    /* TODO check for maximum length */

    header_length=Websocket_header_length((uint8_t *) data, avail);
    data_length=smaller_packet_length-header_length;
    dataU = (uint8_t *)IOBuf_read_all(conn->iob,header_length,8*CLIENT_READ_RETRIES);
    memcpy(key,dataU+header_length-4,4);

    flags=dataU[0];
    if (payload==NULL) {
        inprogFlags=flags;
    }
    

    fin=(WS_fin(dataU));
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
            Request_set(conn->req,bfromcstr("FLAGS"),bformat("0x%X",flags|0x80),1);
            rc = Connection_send_to_handler(conn, conn->handler, (void *)dataU,data_length);
            check_debug(rc == 0, "Failed to deliver to the handler.");
    }
    else {
        if(fin) {
            Request_set(conn->req,bfromcstr("FLAGS"),bformat("0x%X",inprogFlags|0x80),1);
        }
        if (payload == NULL) {
            if (fin) {
                rc = Connection_send_to_handler(conn, conn->handler, (void *)dataU,data_length);
                check_debug(rc == 0, "Failed to deliver to the handler.");
            }
            else {
                payload = blk2bstr(dataU,data_length);
                check(payload != NULL,"Allocation failed");
            }
        } else {
            check(BSTR_OK == bcatblk(payload,dataU,data_length), "Concatenation failed");
            if (fin) {
                rc = Connection_send_to_handler(conn, conn->handler, bdata(payload),blength(payload));
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



void Connection_destroy(Connection *conn)
{
    if(conn) {
        Request_destroy(conn->req);
        conn->req = NULL;
        if(conn->client) free(conn->client);
        while(conn->deliverAck!=conn->deliverPost)
        {
            bdestroy(Connection_deliver_dequeue(conn));
        }
        Connection_deliver_enqueue(conn,NULL);
        taskyield();
        IOBuf_destroy(conn->iob);
        IOBuf_destroy(conn->proxy_iob);
        free(conn);
    }
}


Connection *Connection_create(Server *srv, int fd, int rport,
                              const char *remote)
{
    Connection *conn = calloc(sizeof(Connection),1);
    check_mem(conn);

    conn->req = Request_create();
    conn->proxy_iob = NULL;
    conn->rport = rport;
    conn->client = NULL;
    conn->close = 0;
    conn->type = 0;
    conn->filter_state = NULL;

    memcpy(conn->remote, remote, IPADDR_SIZE);
    conn->remote[IPADDR_SIZE] = '\0';

    conn->handler = NULL;

    check_mem(conn->req);

    if(srv != NULL && srv->use_ssl) {
        conn->iob = IOBuf_create(BUFFER_SIZE, fd, IOBUF_SSL);
        check(conn->iob != NULL, "Failed to create the SSL IOBuf.");

        ssl_set_own_cert(&conn->iob->ssl, &srv->own_cert, &srv->rsa_key);
        ssl_set_dh_param(&conn->iob->ssl, srv->dhm_P, srv->dhm_G);
        ssl_set_ciphersuites(&conn->iob->ssl, srv->ciphers);
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
    State_exec(&conn->state, CLOSE, (void *)conn);
    Connection_destroy(conn);
    taskexit(0);
}

int Connection_deliver_raw_internal(Connection *conn, bstring buf)
{
    return IOBuf_send_all(conn->iob, bdata(buf), blength(buf));
}

int Connection_deliver_raw(Connection *conn, bstring buf)
{
    return Connection_deliver_enqueue(conn, bstrcpy(buf));
}

int Connection_deliver(Connection *conn, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bBase64Encode(buf);
    check(b64_buf != NULL, "Failed to base64 encode data.");
    check(conn->iob != NULL, "There's no IOBuffer to send to, Tell Zed.");
    /* The deliver task will free the buffer */
    rc = Connection_deliver_enqueue(conn,b64_buf);
    check_debug(rc == 0, "Failed to write message to conn %d", IOBuf_fd(conn->iob));

    return 0;

error:
    bdestroy(b64_buf);
    return -1;
}


void Connection_deliver_task(void *v)
{
    Connection *conn=v;
    bstring msg=NULL;
    while(1) {
        msg = Connection_deliver_dequeue(conn);
        check_debug(msg,"Received NULL msg on FD %d, exiting deliver task",IOBuf_fd(conn->iob));
        check(-1 != Connection_deliver_raw_internal(conn,msg),"Error delivering to MSG listener on FD %d, closing them.", IOBuf_fd(conn->iob));
        bdestroy(msg);
        msg=NULL;
    }
error:
    bdestroy(msg);
    if (IOBuf_fd(conn->iob) >= 0)
        shutdown(IOBuf_fd(conn->iob), SHUT_RDWR);
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

    // add the x-forwarded-for header
    Request_set(conn->req, &HTTP_X_FORWARDED_FOR, bfromcstr(conn->remote), 1);

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

    IO_SSL_VERIFY_METHOD = Setting_get_int("ssl.verify_optional", 0) ? SSL_VERIFY_OPTIONAL : SSL_VERIFY_NONE;

}


