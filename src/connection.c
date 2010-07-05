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
#include <response.h>

struct tagbstring PING_PATTERN = bsStatic("@[a-z/]- {\"type\":%s*\"ping\"}");

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

    int rc = Response_send_socket_policy(conn->fd);
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

    bstring path = Request_path(conn->req);

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

    Backend *found = Host_match(host, path);
    check(found, "Handler not found: %s", bdata(path));

    Request_set_action(conn->req, found);
    conn->req->target_host = host;

    bdestroy(host_name);
    return Connection_backend_event(found);

error:
    // TODO: need to get the error state resolver working, but this is alright
    bdestroy(host_name);
    Response_send_error(conn->fd, &HTTP_404);
    return CLOSE;
}



int connection_msg_to_handler(State *state, int event, void *data)
{
    TRACE(msg_to_handler);
    Connection *conn = (Connection *)data;
    Handler *handler = Request_get_action(conn->req, handler);
    int rc = 0;

    if(handler) {
        rc = Handler_deliver(handler->send_socket, conn->fd, conn->buf, conn->nread);
        check(rc != -1, "Failed to deliver to handler: %s", 
                bdata(Request_path(conn->req)));
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
    bstring path = Request_path(conn->req);

    debug("GETTING FILE FOR PATH: %s", bdata(path));

    Dir *dir = Request_get_action(conn->req, dir);

    int rc = Dir_serve_file(dir, path, conn->fd);

    check(rc == 0, "Failed to serve file.");

    return RESP_SENT;

error:
    Response_send_error(conn->fd, &HTTP_404);
    return CLOSE;
}




int connection_http_to_proxy(State *state, int event, void *data)
{
    TRACE(http_to_proxy);
    Connection *conn = (Connection *)data;
    Proxy *proxy = Request_get_action(conn->req, proxy);
    conn->proxy = NULL;
    ProxyConnect *to_proxy = NULL;
    ProxyConnect *to_listener = NULL;

    to_proxy = Proxy_connect_backend(proxy, conn->fd);

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



int connection_proxy_deliver(State *state, int event, void *data)
{
    TRACE(proxy_deliver);
    Connection *conn = (Connection *)data;
    ProxyConnect *to_proxy = conn->proxy;
    int rc = 0;

    int header_len = Request_header_length(conn->req);
    int content_len = Request_content_length(conn->req);
    int total_len = header_len + content_len;

    if(total_len < conn->nread) {
        debug("!!! UNTESTED BRANCH: total=%d, nread=%d", total_len, conn->nread);
        // we read this request and potentially another
        // send the first request, then move the trailing bit down
        // TODO: do we need the to_proxy buf at all then?
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


int connection_proxy_parse(State *state, int event, void *data)
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
    check(Request_is_http(conn->req), "Someone tried to change the protocol on us from HTTP.");

    // do a light find of this request compared to the last one
    if(!biseq(host, Request_get(conn->req, &HTTP_HOST))) {
        // TODO: this totally won't work, need to probably find the new host and set it
        bdestroy(host);
        return PROXY;
    } else {
        bdestroy(host);

        // query up the path to see if it gets the current request action
        Backend *found = Host_match(target_host, Request_path(conn->req));
        check(found, "Didn't find next target in proxy chain request.");

        if(found != req_action) {
            Request_set_action(conn->req, found);
            return Connection_backend_event(found);
        } else {
            // TODO: since we found it already, keep it set and reuse
            return HTTP_REQ;
        }
    }

    sentinel("Should all be handled in if-statement above.");
error:
    return REMOTE_CLOSE;
}



int connection_proxy_failed(State *state, int event, void *data)
{
    TRACE(proxy_failed);
    Connection *conn = (Connection *)data;

    Response_send_error(conn->fd, &HTTP_502);

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
    .msg_to_directory = connection_msg_to_directory,
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


int Connection_read_header(Connection *conn, Request *req)
{
    int finished = 0;
    int n = 0;
    conn->nparsed = 0;

    Request_start(req);

     while(!finished && conn->nread < BUFFER_SIZE) {
        n = fdread(conn->fd, conn->buf, BUFFER_SIZE - 1 - conn->nread);
        check(n > 0, "Failed to read from socket after %d read: %d parsed.",
                conn->nread, (int)conn->nparsed);

        conn->nread += n;

        check(conn->nread < BUFFER_SIZE, "Read too much, FATAL error: nread: %d, buffer size: %d", conn->nread, BUFFER_SIZE);

        finished = Request_parse(req, conn->buf, conn->nread, &conn->nparsed);

        check(finished != -1, "Error in parsing: %d, bytes: %d, value: %.*s", 
                finished, conn->nread, conn->nread, conn->buf);
    }

    conn->buf[BUFFER_SIZE] = '\0';  // always cap it off

    Request_dump(req);
    
    return conn->nread; 

error:
    return -1;

}

