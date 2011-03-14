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

struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":\\s*\"ping\"}");

struct tagbstring POLICY_XML_REQUEST = bsStatic("<policy-file-request");

int MAX_CONTENT_LENGTH = 20 * 1024;
int BUFFER_SIZE = 4 * 1024;
int CONNECTION_STACK = 32 * 1024;
int CLIENT_READ_RETRIES = 5;


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



int connection_send_socket_response(Connection *conn)
{
    if(Response_send_socket_policy(conn) > 0) {
        return RESP_SENT;
    } else {
        return CLOSE;
    }
}


int connection_route_request(Connection *conn)
{
    Host *host = NULL;
    Route *route = NULL;

    bstring path = Request_path(conn->req);

    if(conn->req->host_name) {
        host = Server_match_backend(conn->server, conn->req->host_name);
    } else {
        host = conn->server->default_host;
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

        bstring payload = Request_to_payload(conn->req, handler->send_ident,
                IOBuf_fd(conn->iob), IOBuf_start(conn->iob) + header_len,
                body_len - 1);  // drop \0 on payloads


        rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
        bdestroy(payload);
    
        check(rc == 0, "Failed to deliver to handler: %s", bdata(Request_path(conn->req)));
    }

    // consumes \0 from body_len
    check(IOBuf_read_commit(conn->iob, header_len + body_len) != -1, "Final commit failed.");

    return REQ_SENT;

error:
    return CLOSE;
}


int Connection_send_to_handler(Connection *conn, Handler *handler, char *body, int content_len)
{
    int rc = 0;
    bstring result = NULL;

    result = Request_to_payload(conn->req, handler->send_ident, 
            IOBuf_fd(conn->iob), body, content_len);
    check(result, "Failed to create payload for request.");

    debug("HTTP TO HANDLER: %.*s", blength(result) - content_len, bdata(result));

    rc = Handler_deliver(handler->send_socket, bdata(result), blength(result));
    error_unless(rc != -1, conn, 502, "Failed to deliver to handler: %s", 
            bdata(Request_path(conn->req)));

    bdestroy(result);
    return 0;

error:
    bdestroy(result);
    return -1;
}


static struct tagbstring UPGRADE = bsStatic("upgrade");
static struct tagbstring CONNECTION = bsStatic("connection");
const int WEBSOCKET_ARBITRARY_BODY_SIZE = 8;

static inline int is_websocket(Connection *conn)
{
    return Request_get(conn->req, &UPGRADE) != NULL && 
        Request_get(conn->req, &CONNECTION) != NULL;
}

int connection_http_to_handler(Connection *conn)
{
    int content_len = Request_content_length(conn->req);
    int rc = 0;
    char *body = NULL;

    Handler *handler = Request_get_action(conn->req, handler);
    error_unless(handler, conn, 404, "No action for request: %s", bdata(Request_path(conn->req)));

    // we don't need the header anymore, so commit the buffer and deal with the body
    check(IOBuf_read_commit(conn->iob, Request_header_length(conn->req)) != -1, "Finaly commit failed streaming the connection to http handlers.");

    // WebSocket detection
    if(is_websocket(conn)) {
        content_len = IOBuf_avail(conn->iob);
        check(content_len == WEBSOCKET_ARBITRARY_BODY_SIZE, "Purported websocket but body does not have 8 bytes");
    }

    if(content_len == 0) {
        body = "";
        rc = Connection_send_to_handler(conn, handler, body, content_len);
        check_debug(rc == 0, "Failed to deliver to the handler.");
    } else if(content_len > MAX_CONTENT_LENGTH) {
        rc = Upload_file(conn, handler, content_len);
        check(rc == 0, "Failed to upload file.");
    } else {
        if(content_len > conn->iob->len) {
            // temporarily grow the buffer
            IOBuf_resize(conn->iob, content_len);
        }

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

    int proxy_fd = netdial(1, bdata(proxy->server), proxy->port);
    check(proxy_fd != -1, "Failed to connect to proxy backend %s:%d",
            bdata(proxy->server), proxy->port);

    if(!conn->proxy_iob) {
        conn->proxy_iob = IOBuf_create(BUFFER_SIZE, proxy_fd, IOBUF_SOCKET);
        check_mem(conn->proxy_iob);
    }

    if(!conn->client) {
        conn->client = h_calloc(sizeof(httpclient_parser), 1);
        check_mem(conn->client);
        hattach(conn->client, conn);
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


static inline int ident_and_register(Connection *conn, int reg_too)
{
    int next = CLOSE;

    if(Request_is_xml(conn->req)) {
        if(biseq(Request_path(conn->req), &POLICY_XML_REQUEST)) {
            debug("XML POLICY CONNECTION");
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
    } else if(Request_is_http(conn->req)) {
        debug("HTTP MESSAGE");
        conn->type = CONN_TYPE_HTTP;
        taskname("HTTP");
        next = HTTP_REQ;
    } else {
        error_response(conn, 500, "Invalid code branch, tell Zed.");
    }

    if(reg_too) Register_connect(IOBuf_fd(conn->iob), (void*)conn);
    return next;

error:
    return CLOSE;

}


int connection_register_request(Connection *conn)
{
    return ident_and_register(conn, 1);
}

int connection_identify_request(Connection *conn)
{
    return ident_and_register(conn, 0);
}

int connection_parse(Connection *conn)
{
    if(Connection_read_header(conn, conn->req) > 0) {
        return REQ_RECV;
    } else {
        return CLOSE;
    }
}

StateActions CONN_ACTIONS = {
    .error = connection_error,
    .close = connection_close,
    .parse = connection_parse,
    .register_request = connection_register_request,
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
    .proxy_close = connection_proxy_close
};



void Connection_destroy(Connection *conn)
{
    if(conn) {
        Request_destroy(conn->req);
        conn->req = NULL;
        IOBuf_destroy(conn->iob);
        IOBuf_destroy(conn->proxy_iob);
        h_free(conn);
    }
}


Connection *Connection_create(Server *srv, int fd, int rport, 
                              const char *remote, SSL_CTX *ssl_ctx)
{
    Connection *conn = h_calloc(sizeof(Connection), 1);
    check_mem(conn);

    conn->server = srv;

    conn->rport = rport;
    memcpy(conn->remote, remote, IPADDR_SIZE);
    conn->remote[IPADDR_SIZE] = '\0';
    conn->type = 0;

    conn->req = Request_create();
    check_mem(conn->req);

    if(ssl_ctx != NULL) {
        conn->iob = IOBuf_create(BUFFER_SIZE, fd, IOBUF_SSL);
        check(conn->iob != NULL, "Failed to create the SSL IOBuf.");
        conn->iob->ssl = ssl_server_new(ssl_ctx, IOBuf_fd(conn->iob));
        check(conn->iob->ssl != NULL, "Failed to create new ssl for connection");
    } else {
        conn->iob = IOBuf_create(BUFFER_SIZE, fd, IOBUF_SOCKET);
    }

    return conn;

error:
    Connection_destroy(conn);
    return NULL;
}


void Connection_accept(Connection *conn)
{
    taskcreate(Connection_task, conn, CONNECTION_STACK);
}



void Connection_task(void *v)
{
    Connection *conn = (Connection *)v;
    int i = 0;
    int next = 0;

    State_init(&conn->state, &CONN_ACTIONS);

    for(i = 0, next = OPEN; next != CLOSE; i++) {
        next = State_exec(&conn->state, next, (void *)conn);

        check(next >= CLOSE && next < EVENT_END,
                "!!! Invalid next event[%d]: %d, Tell ZED!", i, next);
    }

error: // fallthrough
    State_exec(&conn->state, CLOSE, (void *)conn);
    Connection_destroy(conn);
    return;
}

int Connection_deliver_raw(Connection *conn, bstring buf)
{
    return IOBuf_send(conn->iob, bdata(buf), blength(buf));
}

int Connection_deliver(Connection *conn, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bBase64Encode(buf);
    rc = IOBuf_send(conn->iob, bdata(b64_buf), blength(b64_buf)+1);
    check_debug(rc == blength(b64_buf)+1, "Failed to write entire message to conn %d", IOBuf_fd(conn->iob));

    bdestroy(b64_buf);
    return 0;

error:
    bdestroy(b64_buf);
    return -1;
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
    Request_set(conn->req, bstrcpy(&HTTP_X_FORWARDED_FOR),
            blk2bstr(conn->remote, IPADDR_SIZE), 1);

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
}


