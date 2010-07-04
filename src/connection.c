#include <connection.h>
#include <host.h>
#include <http11/http11_parser.h>
#include <bstring.h>
#include <dbg.h>
#include <task/task.h>
#include <events.h>
#include <register.h>
#include <b64/b64.h>
#include <handler.h>
#include <pattern.h>
#include <dir.h>
#include <proxy.h>
#include <assert.h>
#include <sys/socket.h>


struct tagbstring FLASH_RESPONSE = bsStatic("<?xml version=\"1.0\"?>"
        "<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">"
        "<cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>");


struct tagbstring HTTP_404 = bsStatic("HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 9\r\n"
    "Server: Mongrel2\r\n\r\nNot Found");

struct tagbstring HTTP_502 = bsStatic("HTTP/1.1 502 Bad Gateway\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 11\r\n"
    "Server: Mongrel2\r\n\r\nBad Gateway");

struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":%s*\"ping\"}");

struct tagbstring HTTP_HOST = bsStatic("HOST");


#define TRACE(C) debug("--> %s(%s:%d) %s:%d ", "" #C, State_event_name(event), event, __FUNCTION__, __LINE__)


inline int Connection_backend_event(Backend *found)
{
    switch(found->type) {
        case BACKEND_HANDLER:
            return HANDLER;
        case BACKEND_DIR:
            return DIRECTORY;
        case BACKEND_PROXY:
            return PROXY;
        default:
            return CLOSE;
    }
}


int connection_open(State *state, int event, void *data)
{
    TRACE(open);
    Connection *conn = (Connection *)data;

    if(!conn->registered) {
        Register_connect(conn->fd);
        conn->registered = 1;
    }

    return ACCEPT;
}


int connection_error(State *state, int event, void *data)
{
    TRACE(error);
    Connection *conn = (Connection *)data;

    Register_disconnect(conn->fd);
    fdclose(conn->fd);

    return CLOSE;
}


int connection_finish(State *state, int event, void *data)
{
    TRACE(finish);

    Connection_destroy((Connection *)data);

    return CLOSE;
}


int connection_close(State *state, int event, void *data)
{
    TRACE(close);

    Connection *conn = (Connection *)data;

    Register_disconnect(conn->fd);

    fdclose(conn->fd);

    return 0;
}



int connection_send_socket_response(State *state, int event, void *data)
{
    TRACE(socket_req);
    Connection *conn = (Connection *)data;

    int rc = fdsend(conn->fd, bdata(&FLASH_RESPONSE),
            blength(&FLASH_RESPONSE) + 1);

    check(rc > 0, "Failed to write Flash socket response.");

    return RESP_SENT;

error:

    return CLOSE;
}


int connection_route_request(State *state, int event, void *data)
{
    TRACE(route);
    Connection *conn = (Connection *)data;
    Host *host = NULL;
    bstring host_name = NULL;

    bstring path = conn->req->path;

    // TODO: pre-process these out since we'll have to look them up all the damn time
    bstring host_header = Request_get(conn->req, &HTTP_HOST);
    host_name = bHead(host_header, bstrchr(host_header, ':'));

    if(host_name) {
        host = Server_match(conn->server, host_name);
        // TODO: find out if this should be an error or not
        check(host, "Request for a host we don't have registered: %s", bdata(host_name));
    } else {
        host = conn->server->default_host;
    }
    check(host, "Failed to resolve a host for the request, set a default host.");

    Backend *found = Host_match(conn->server->default_host, path);
    check(found, "Handler not found: %s", bdata(path));

    conn->req->action = found;

    bdestroy(host_name);
    return Connection_backend_event(found);

error:
    // TODO: need to get the error state resolver working, but this is alright
    bdestroy(host_name);
    fdsend(conn->fd, bdata(&HTTP_404), blength(&HTTP_404));
    return CLOSE;
}



int connection_msg_to_handler(State *state, int event, void *data)
{
    TRACE(msg_to_handler);
    Connection *conn = (Connection *)data;
    Handler *handler = conn->req->action->target.handler;
    int rc = 0;

    if(handler) {
        rc = Handler_deliver(handler->send_socket, conn->fd, conn->buf, conn->nread);
        check(rc != -1, "Failed to deliver to handler: %s", bdata(conn->req->path));
    }

    // TODO: do an error instead of just allowing NULL handlers
    return REQ_SENT;

error:
    return CLOSE;
}



int connection_msg_to_directory(State *state, int event, void *data)
{
    TRACE(msg_to_directory);

    log_err("MSG to DIR currently not supported.");

    return CLOSE;
}



int connection_http_to_handler(State *state, int event, void *data)
{
    TRACE(http_to_handler);

    log_err("HTTP to HANDLER is not supported yet.");

    return CLOSE;
}



int connection_http_to_directory(State *state, int event, void *data)
{
    TRACE(http_to_directory);
    Connection *conn = (Connection *)data;
    bstring path = conn->req->path;

    Dir *dir = conn->req->action->target.dir;

    int rc = Dir_serve_file(dir, path, conn->fd);

    check(rc == 0, "Failed to serve file.");

    return RESP_SENT;

error:
    fdsend(conn->fd, bdata(&HTTP_404), blength(&HTTP_404));
    return CLOSE;
}




int connection_http_to_proxy(State *state, int event, void *data)
{
    TRACE(http_to_proxy);
    Connection *conn = (Connection *)data;
    Proxy *proxy = conn->req->action->target.proxy;
    conn->proxy = NULL;
    ProxyConnect *to_proxy = NULL;
    ProxyConnect *to_listener = NULL;

    to_proxy = Proxy_connect_backend(proxy, conn->fd, conn->buf, BUFFER_SIZE, conn->nread);

    check(to_proxy, "Failed to connect to backend proxy server: %s:%d",
            bdata(proxy->server), proxy->port);

    to_listener = Proxy_sync_to_listener(to_proxy);
    check(to_listener, "Failed to make the listener side of proxy.");

    // we keep this since we manage it
    conn->proxy = to_proxy;

    return CONNECT;

error:
    if(to_listener && to_proxy) fdclose(to_proxy->proxy_fd);
    ProxyConnect_destroy(to_listener);
    return FAILED;
}


int connection_proxy_connected(State *state, int event, void *data)
{
    TRACE(proxy_to_proxy);
    Connection *conn = (Connection *)data;
    ProxyConnect *to_proxy = conn->proxy;
    int rc = 0;

    /*
     * Now we have to change this so we base our send/recv loop not
     * on raw bytes, but on parsing the request.
     *
     * First, we should have a request ready to go, so send the headers,
     * look at the clength, and send the remaining body.
     *
     * Second, after sending that request, we read a new request, and
     * see how it routes.  If it routes the same as the original, then
     * we send this new request on just like normal.
     *
     * Third, if the new request is different, then nuke the original,
     * set it to the original, and return the proper type (HANDLER) for
     * the Connection::Idle state.
     */

    debug("CONTENT LENGTH: %d", conn->req->parser.content_len);
    int header_len = conn->req->parser.body_start;
    int len_remaining = conn->req->parser.content_len + header_len;

    // totally inneficient here, but send the header
    rc = fdsend(to_proxy->proxy_fd, to_proxy->buffer, header_len);
    check(rc > 0, "Failed to write header to proxy.");
    len_remaining -= rc;

    // then send the body if we have any
    if(to_proxy->n - header_len > 0) {
        rc = fdsend(to_proxy->proxy_fd, to_proxy->buffer+header_len, to_proxy->n - header_len);
        check(rc > 0, "Failed to write trailing body piece: %d out of %d", rc, to_proxy->n - header_len);
        len_remaining -= rc;
    }

    for(to_proxy->n = 0; len_remaining > 0; len_remaining -= rc)
    {
        to_proxy->n = fdrecv(to_proxy->client_fd, to_proxy->buffer, to_proxy->size);
        check(to_proxy->n > 0, "Failed to read from client.");

        rc = fdsend(to_proxy->proxy_fd, to_proxy->buffer, to_proxy->n);
        check(rc > 0, "Failed to write to proxy.");
    }

    debug("REMAINING: %d out of (%d header, %d body)", len_remaining, 
            conn->req->parser.body_start, conn->req->parser.content_len);

    shutdown(to_proxy->proxy_fd, SHUT_WR);

    taskbarrier(to_proxy->waiter);

    return REMOTE_CLOSE;

error:
    // TODO: look at returning FAILED here instead
    return REMOTE_CLOSE;
}


int connection_proxy_failed(State *state, int event, void *data)
{
    TRACE(proxy_failed);
    Connection *conn = (Connection *)data;

    int rc = fdsend(conn->fd, bdata(&HTTP_502), blength(&HTTP_502));
    debug("SEND RC: %d", rc);

    ProxyConnect_destroy(conn->proxy);

    return CLOSE;
}


int connection_proxy_close(State *state, int event, void *data)
{
    TRACE(proxy_close);

    ProxyConnect *to_proxy = ((Connection *)data)->proxy;

    fdclose(to_proxy->proxy_fd);

    taskbarrier(to_proxy->waiter);

    ProxyConnect_destroy(to_proxy);

    return CLOSE;
}



int connection_identify_request(State *state, int event, void *data)
{
    Connection *conn = (Connection *)data;

    TRACE(ident_req);

    if(Request_is_socket(conn->req)) {
        return SOCKET_REQ;
    } else if(Request_is_json(conn->req)) {
        return MSG_REQ;
    } else if(Request_is_http(conn->req)) {
        return HTTP_REQ;
    } else {
        sentinel("Invalid request type, TELL ZED.");
    }

error:
    return CLOSE;
}



int connection_parse(State *state, int event, void *data)
{
    Connection *conn = (Connection *)data;
    int finished = 0;
    conn->nread = 0;
    conn->nparsed = 0;
    int n = 0;

    TRACE(parse);

    debug("PARSING with nread: %d and nparser: %d", conn->nread, (int)conn->nparsed);

    Request_start(conn->req);

    do {
        n = fdread(conn->fd, conn->buf, BUFFER_SIZE - conn->nread -1);
        check(n > 0, "Failed to read from socket after %d read: %d parsed.",
                conn->nread, (int)conn->nparsed);

        conn->nread += n;

        finished = Request_parse(conn->req, conn->buf, conn->nread, &conn->nparsed);

        check(finished != -1, "Error in parsing: %d, bytes: %d, value: %.*s", 
                finished, conn->nread, conn->nread, conn->buf);

    } while(!finished && conn->nread < BUFFER_SIZE);

    return REQ_RECV;
    
error:

    return CLOSE;
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
    .msg_to_directory = connection_msg_to_directory,
    .http_to_handler = connection_http_to_handler,
    .http_to_proxy = connection_http_to_proxy,
    .http_to_directory = connection_http_to_directory,
    .proxy_connected = connection_proxy_connected,
    .proxy_failed = connection_proxy_failed,
    .proxy_parse = connection_parse,
    .proxy_close = connection_proxy_close
};



void Connection_destroy(Connection *conn)
{
    if(conn) {
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
        debug("NEXT EVENT: %s:%d", State_event_name(next), next);

        next = State_exec(&conn->state, next, (void *)conn);
        check(next > EVENT_START && next < EVENT_END, "!!! Invalid next event[%d]: %d", i, next);
    }

    debug("CONNECTION DONE");
    State_exec(&conn->state, CLOSE, (void *)conn);
    return;

error:
    State_exec(&conn->state, CLOSE, (void *)conn);
    debug("!!! CONNECTION ERROR.");
    return;
}



int Connection_deliver(int to_fd, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bfromcstralloc(blength(buf) * 2, "");

    b64_buf->slen = b64_encode(bdata(buf), blength(buf), bdata(b64_buf), blength(buf) * 2 - 1); 
    b64_buf->data[b64_buf->slen] = '\0';

    rc = fdsend(to_fd, bdata(b64_buf), blength(b64_buf)+1);

    check(rc == blength(b64_buf)+1, "Failed to write entire message to conn %d", to_fd);

    bdestroy(b64_buf);
    return 0;

error:
    bdestroy(b64_buf);
    return -1;
}

