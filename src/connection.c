#include <connection.h>
#include <host.h>
#include <http11/http11_parser.h>
#include <bstring.h>
#include <dbg.h>
#include <task/task.h>
#include <events.h>
#include <register.h>
#include <handler.h>
#include <pattern.h>
#include <dir.h>
#include <proxy.h>
#include <assert.h>
#include <sys/socket.h>
#include <response.h>

struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":%s*\"ping\"}");

#define TRACE(C) debug("--> %s(%s:%d) %s:%d ", "" #C, State_event_name(event), event, __FUNCTION__, __LINE__)

#define ERROR(F, C, M, ...)  {Response_send_status(F, &HTTP_##C); sentinel(M, ##__VA_ARGS__);}
#define ERROR_UNLESS(T, F, C, M, ...) if(!(T)) ERROR(F, C, M, ##__VA_ARGS__)

enum {
    MAX_CONTENT_LENGTH = 20 * 1024
};

inline int Connection_backend_event(Backend *found, int fd)
{
    switch(found->type) {
        case BACKEND_HANDLER:
            return HANDLER;
        case BACKEND_DIR:
            return DIRECTORY;
        case BACKEND_PROXY:
            return PROXY;
        default:
            ERROR(fd, 501, "Invalid backend type: %d", found->type);
    }

error:
    return CLOSE;
}


int connection_open(int event, void *data)
{
    TRACE(open);
    Connection *conn = (Connection *)data;

    if(!conn->registered) {
        conn->registered = 1;
    }

    return ACCEPT;
}



int connection_finish(int event, void *data)
{
    TRACE(finish);

    Connection_destroy((Connection *)data);

    return CLOSE;
}



int connection_send_socket_response(int event, void *data)
{
    TRACE(socket_req);
    Connection *conn = (Connection *)data;

    int rc = Response_send_socket_policy(conn->fd);
    check(rc > 0, "Failed to write Flash socket response.");

    return RESP_SENT;

error:
    return CLOSE;
}


int connection_route_request(int event, void *data)
{
    TRACE(route);
    Connection *conn = (Connection *)data;
    Host *host = NULL;
    bstring host_name = NULL;

    bstring path = Request_path(conn->req);

    // TODO: pre-process these out since we'll have to look them up all the damn time
    bstring host_header = Request_get(conn->req, &HTTP_HOST);
    host_name = bHead(host_header, bstrchr(host_header, ':'));

    if(host_name) {
        host = Server_match_backend(conn->server, host_name);
    } else {
        host = conn->server->default_host;
    }
    ERROR_UNLESS(host, conn->fd, 404, "Request for a host we don't have registered: %s", bdata(host_name));

    Backend *found = Host_match_backend(host, path);
    ERROR_UNLESS(found, conn->fd, 404, "Handler not found: %s", bdata(path));

    Request_set_action(conn->req, found);
    conn->req->target_host = host;

    bdestroy(host_name);
    return Connection_backend_event(found, conn->fd);

error:
    bdestroy(host_name);
    return CLOSE;
}



int connection_msg_to_handler(int event, void *data)
{
    TRACE(msg_to_handler);
    Connection *conn = (Connection *)data;
    Handler *handler = Request_get_action(conn->req, handler);
    int rc = 0;

    check(handler, "JSON request doesn't match any handler: %s", 
            bdata(Request_path(conn->req)));

    if(pattern_match(conn->buf, conn->nparsed, bdata(&PING_PATTERN))) {
        Register_ping(conn->fd);
    } else {
        // TODO: Get ragel to do this, not us.
        int header_len = blength(Request_path(conn->req)) + 1;
        check(conn->nread - header_len - 1 >= 0, "Header length calculation is wrong.");

        bstring payload = Request_to_payload(conn->req, handler->send_ident,
                conn->fd, conn->buf + header_len, conn->nread - header_len - 1);

        rc = Handler_deliver(handler->send_socket, bdata(payload), blength(payload));
        bdestroy(payload);
    
        ERROR_UNLESS(rc != -1, conn->fd, 502, "Failed to deliver to handler: %s", 
                bdata(Request_path(conn->req)));
    }

    return REQ_SENT;

error:
    return CLOSE;
}


int connection_http_to_handler(int event, void *data)
{
    TRACE(http_to_handler);
    Connection *conn = (Connection *)data;
    int content_len = Request_content_length(conn->req);
    int header_len = Request_header_length(conn->req);
    int total = header_len + content_len;
    int rc = 0;
    char *body = NULL;
    bstring result = NULL;

    Handler *handler = Request_get_action(conn->req, handler);
    check(handler, "No action for request: %s", bdata(Request_path(conn->req)));

    if(content_len == 0) {
        body = "";
    } else if(content_len > MAX_CONTENT_LENGTH) {
        ERROR(conn->fd, 413, "Request entity is too large: %d", content_len);    
    } else {
        if(total > BUFFER_SIZE) {
            conn->buf = realloc(conn->buf, total);
            body = conn->buf + header_len;
            rc = fdread(conn->fd, body, content_len);
            check(rc == content_len, "Failed to read %d content as requested.", content_len);
        } else {
            // got it already so just start off where it is
            body = conn->buf + header_len;
        }
    }

    result = Request_to_payload(conn->req, handler->send_ident, conn->fd, body, content_len);
    check(result, "Failed to create payload for request.");

    rc = Handler_deliver(handler->send_socket, bdata(result), blength(result));
    ERROR_UNLESS(rc != -1, conn->fd, 502, "Failed to deliver to handler: %s", 
            bdata(Request_path(conn->req)));

    bdestroy(result);
    return REQ_SENT;

error:
    bdestroy(result);
    return CLOSE;
}



int connection_http_to_directory(int event, void *data)
{
    TRACE(http_to_directory);
    Connection *conn = (Connection *)data;
    bstring path = Request_path(conn->req);

    Dir *dir = Request_get_action(conn->req, dir);

    int rc = Dir_serve_file(conn->req, dir, path, conn->fd);
    check(rc == 0, "Failed to serve file: %s", bdata(path));

    return RESP_SENT;

error:
    return CLOSE;
}




int connection_http_to_proxy(int event, void *data)
{
    TRACE(http_to_proxy);
    Connection *conn = (Connection *)data;
    Proxy *proxy = Request_get_action(conn->req, proxy);
    ProxyConnect *to_listener = NULL;

    conn->proxy = Proxy_connect_backend(proxy, conn->fd);
    check(conn->proxy, "Failed to connect to backend proxy server: %s:%d",
            bdata(proxy->server), proxy->port);

    to_listener = Proxy_sync_to_listener(conn->proxy);
    check(to_listener, "Failed to make the listener side of proxy.");

    return CONNECT;

error:
    if(to_listener && conn->proxy) fdclose(conn->proxy->proxy_fd);
    ProxyConnect_destroy(to_listener);
    return FAILED;
}



int connection_proxy_deliver(int event, void *data)
{
    TRACE(proxy_deliver);
    Connection *conn = (Connection *)data;
    ProxyConnect *to_proxy = conn->proxy;
    int rc = 0;

    int total_len = Request_header_length(conn->req) + Request_content_length(conn->req);

    if(total_len < conn->nread) {
        debug("!!! UNTESTED BRANCH: total=%d, nread=%d", total_len, conn->nread);
        rc = fdwrite(to_proxy->proxy_fd, conn->buf, total_len);
        check(rc > 0, "Failed to write request to proxy.");

        // setting up for the next request to be read
        conn->nread -= total_len;
        memmove(conn->buf, conn->buf + total_len, conn->nread);
    } else if (total_len > conn->nread) {
        // we haven't read everything, need to do some streaming
        do {
            // TODO: look at sendfile or splice to do this instead
            rc = fdsend(to_proxy->proxy_fd, conn->buf, conn->nread);
            check(rc == conn->nread, "Failed to write full request to proxy after %d read.", conn->nread);

            total_len -= rc;

            if(total_len > 0) {
                conn->nread = fdrecv(conn->fd, conn->buf, BUFFER_SIZE);
                check(conn->nread > 0, "Failed to read from client more data with %d left.", total_len);
            } else {
                conn->nread = 0;
            }
        } while(total_len > 0);
    } else {
        // not > and not < means ==, so we just write this and try again
        rc = fdsend(to_proxy->proxy_fd, conn->buf, total_len);
        check(rc == total_len, "Failed to write complete request to proxy, wrote only: %d", rc);
        conn->nread = 0;
    }

    return REQ_SENT;

error:
    return REMOTE_CLOSE;
}


int connection_proxy_parse(int event, void *data)
{
    TRACE(proxy_parse);

    int rc = 0;
    Connection *conn = (Connection *)data;
    bstring host = bstrcpy(Request_get(conn->req, &HTTP_HOST));
    Host *target_host = conn->req->target_host;
    Backend *req_action = conn->req->action;

    // unlike other places, we keep the nread rather than reset
    rc = Connection_read_header(conn, conn->req);
    check(rc > 0, "Failed to read another header.");
    ERROR_UNLESS(Request_is_http(conn->req), conn->fd, 400,
            "Someone tried to change the protocol on us from HTTP.");

    // do a light find of this request compared to the last one
    if(!biseq(host, Request_get(conn->req, &HTTP_HOST))) {
        bdestroy(host);
        return PROXY;
    } else {
        bdestroy(host);

        // query up the path to see if it gets the current request action
        Backend *found = Host_match_backend(target_host, Request_path(conn->req));
        ERROR_UNLESS(found, conn->fd, 404, 
                "Handler not found: %s", bdata(Request_path(conn->req)));

        if(found != req_action) {
            Request_set_action(conn->req, found);
            return Connection_backend_event(found, conn->fd);
        } else {
            // TODO: since we found it already, keep it set and reuse
            return HTTP_REQ;
        }
    }

    ERROR(conn->fd, 500, "Invalid code branch, tell Zed.");
error:
    return REMOTE_CLOSE;
}



int connection_proxy_failed(int event, void *data)
{
    TRACE(proxy_failed);
    Connection *conn = (Connection *)data;

    Response_send_status(conn->fd, &HTTP_502);

    ProxyConnect_destroy(conn->proxy);

    return CLOSE;
}


int connection_proxy_close(int event, void *data)
{
    TRACE(proxy_close);

    Connection *conn = (Connection *)data;

    if(conn->proxy) {
        ProxyConnect *to_proxy = conn->proxy;

        fdclose(to_proxy->proxy_fd);

        // this waits on the task in proxy.c that moves data from the proxy to the client
        taskbarrier(to_proxy->waiter);

        ProxyConnect_destroy(to_proxy);

        conn->proxy = NULL;
    }

    return CLOSE;
}

int connection_close(int event, void *data)
{
    TRACE(close);
    Connection *conn = (Connection *)data;

    if(conn->proxy) {
        connection_proxy_close(event, data);
    }

    Register_disconnect(conn->fd);

    return 0;
}



int connection_error(int event, void *data)
{
    TRACE(error);
    Connection *conn = (Connection *)data;

    if(conn->proxy) {
        connection_proxy_close(event, data);
    }

    Register_disconnect(conn->fd);
    fdclose(conn->fd);

    return CLOSE;
}




int connection_identify_request(int event, void *data)
{
    Connection *conn = (Connection *)data;

    TRACE(ident_req);

    if(Request_is_socket(conn->req)) {
        Register_connect(conn->fd, CONN_TYPE_SOCKET);
        return SOCKET_REQ;
    } else if(Request_is_json(conn->req)) {
        Register_connect(conn->fd, CONN_TYPE_MSG);
        return MSG_REQ;
    } else if(Request_is_http(conn->req)) {
        Register_connect(conn->fd, CONN_TYPE_HTTP);
        return HTTP_REQ;
    } else {
        ERROR(conn->fd, 500, "Invalid code branch, tell Zed.");
    }

error:
    return CLOSE;
}



int connection_parse(int event, void *data)
{
    Connection *conn = (Connection *)data;
    conn->nread = 0;

    if(Connection_read_header(conn, conn->req) > 0) {
        return REQ_RECV;
    } else {
        return CLOSE;
    }
}


StateActions CONN_ACTIONS = {
    .open = connection_open,
    .error = connection_error,
    .finish = connection_finish,
    .close = connection_close,
    .parse = connection_parse,
    .identify_request = connection_identify_request,
    .route_request = connection_route_request,
    .send_socket_response = connection_send_socket_response,
    .msg_to_handler = connection_msg_to_handler,
    .http_to_handler = connection_http_to_handler,
    .http_to_proxy = connection_http_to_proxy,
    .http_to_directory = connection_http_to_directory,
    .proxy_deliver = connection_proxy_deliver,
    .proxy_failed = connection_proxy_failed,
    .proxy_parse = connection_proxy_parse,
    .proxy_close = connection_proxy_close
};



void Connection_destroy(Connection *conn)
{
    if(conn) {
        if(conn->buf) free(conn->buf);
        Request_destroy(conn->req);
        conn->req = NULL;
        free(conn);
    }
}

Connection *Connection_create(Server *srv, int fd, int rport, const char *remote)
{
    Connection *conn = calloc(sizeof(Connection), 1);
    check(conn, "Out of memory.");

    conn->server = srv;
    conn->fd = fd;

    conn->rport = rport;
    memcpy(conn->remote, remote, IPADDR_SIZE);
    conn->remote[IPADDR_SIZE] = '\0';

    conn->req = Request_create();
    check(conn->req, "Failed to allocate Request.");

    conn->buf = malloc(BUFFER_SIZE + 1);
    check(conn->buf, "Failed to allocate buffer size: %d", BUFFER_SIZE + 1);

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
        ERROR_UNLESS(next >= FINISHED && next < EVENT_END, conn->fd, 500, 
                "!!! Invalid next event[%d]: %d, Tell ZED!", i, next);
    }

    State_exec(&conn->state, CLOSE, (void *)conn);
    return;

error:
    State_exec(&conn->state, CLOSE, (void *)conn);
    return;
}

int Connection_deliver_raw(int to_fd, bstring buf)
{
    return fdsend(to_fd, bdata(buf), blength(buf));
}

int Connection_deliver(int to_fd, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bBase64Encode(buf);
    rc = fdsend(to_fd, bdata(b64_buf), blength(b64_buf)+1);
    check(rc == blength(b64_buf)+1, "Failed to write entire message to conn %d", to_fd);

    bdestroy(b64_buf);
    return 0;

error:
    bdestroy(b64_buf);
    return -1;
}


int Connection_read_header(Connection *conn, Request *req)
{
    int finished = 0;
    int n = 0;
    conn->nparsed = 0;

    Request_start(req);

    conn->buf[BUFFER_SIZE] = '\0';  // always cap it off

    while(!finished && conn->nread < BUFFER_SIZE - 1) {
        n = fdread(conn->fd, conn->buf, BUFFER_SIZE - 1 - conn->nread);
        check(n > 0, "Failed to read from socket after %d read: %d parsed.",
                conn->nread, (int)conn->nparsed);

        conn->nread += n;

        check(conn->buf[BUFFER_SIZE] == '\0', "Trailing \\0 was clobbered, buffer overflow potentially.");
        check(conn->nread < BUFFER_SIZE, "Read too much, FATAL error: nread: %d, buffer size: %d", conn->nread, BUFFER_SIZE);

        finished = Request_parse(req, conn->buf, conn->nread, &conn->nparsed);

        ERROR_UNLESS(finished != -1, conn->fd, 400, 
                "Error in parsing: %d, bytes: %d, value: %.*s, parsed: %d", 
                finished, conn->nread, conn->nread, conn->buf, (int)conn->nparsed);

    }

    ERROR_UNLESS(finished, conn->fd, 400, "HEADERS and/or request too big.");

    Request_dump(req);

    return conn->nread; 

error:
    return -1;

}

