#include <listener.h>
#include <string.h>
#include <proxy.h>
#include <zmq.h>
#include <task/task.h>
#include <dbg.h>
#include <handler.h>
#include <dir.h>
#include <request.h>
#include <register.h>
#include <pattern.h>
#include <b64/b64.h>


char *FLASH_RESPONSE = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

size_t FLASH_LEN = 0;

struct tagbstring HTTP_404 = bsStatic("HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 9\r\n"
    "Server: Mongrel2\r\n\r\nNot Found");

const char *PING_PATTERN = "@[a-z/]- {\"type\":%s*\"ping\"}"; 


// these are used by unit tests to fake out sockets from files
static int (*Listener_read_func)(int, void*, int);
static int (*Listener_write_func)(int, void*, int);


void Listener_init()
{
    FLASH_LEN = strlen(FLASH_RESPONSE);
    Listener_set_iofuncs(fdrecv, fdsend);
}

void Listener_set_iofuncs( int (*read_func)(int, void*, int),
        int (*write_func)(int, void*, int))
{
    Listener_read_func = read_func;
    Listener_write_func = write_func;
}


void Listener_accept(Server *srv, int fd, int rport, const char *remote)
{
    Listener *listener = Listener_create(srv, fd, rport, remote);

    taskcreate(Listener_task, listener, LISTENER_STACK);
}


Listener *Listener_create(Server *srv, int fd, int rport, const char *remote)
{
    Listener *listener = calloc(sizeof(Listener), 1);
    check(listener, "Out of memory.");

    listener->server = srv;
    listener->fd = fd;

    listener->rport = rport;
    memcpy(listener->remote, remote, IPADDR_SIZE);
    listener->remote[IPADDR_SIZE] = '\0';

    listener->parser = Request_create();
    check(listener->parser, "Failed to allocate Request.");


    return listener;
error:
    Listener_destroy(listener);
    return NULL;
}

void Listener_destroy(Listener *listener)
{
    if(listener) {
        Request_destroy(listener->parser);
        free(listener);
    }
}


int Listener_deliver(int to_fd, bstring buf)
{
    int rc = 0;

    bstring b64_buf = bfromcstralloc(blength(buf) * 2, "");

    b64_buf->slen = b64_encode(bdata(buf), blength(buf), bdata(b64_buf), blength(buf) * 2 - 1); 
    b64_buf->data[b64_buf->slen] = '\0';

    rc = Listener_write_func(to_fd, bdata(b64_buf), blength(b64_buf)+1);

    check(rc == blength(b64_buf)+1, "Failed to write entire message to listener %d", to_fd);


    bdestroy(b64_buf);
    return 0;

error:
    bdestroy(b64_buf);
    return -1;
}



int Listener_process_json(Listener *listener)
{
    bstring path = bfromcstr("");
    check(listener->server->default_host, "No default host set, need one for jssockets to work.");

    if(!listener->registered) {
        Register_connect(listener->fd);
        listener->registered = 1;
    }

    if(pattern_match(listener->buf, listener->parser->body_start, PING_PATTERN)) {
        if(!Register_ping(listener->fd)) {
            Register_disconnect(listener->fd);
        }

    } else {
        Backend *found = Listener_match_path(listener, path);
        check(found, "Handler not found: %s", bdata(path));
        check(found->type == BACKEND_HANDLER, "Should get a handler.");

        debug("JSON message from %s:%d sent on jssocket: %.*s", listener->remote, listener->rport, listener->nread, listener->buf);

        if(Handler_deliver(found->target.handler->send_socket, listener->fd, listener->buf, listener->nread) == -1) {
            log_err("Can't deliver message to handler.");
       }
    }

    bdestroy(path);
    return 0;

error:
    bdestroy(path);
    return -1;
}


struct tagbstring HTTP_PATH = bsStatic ("PATH");

Backend *Listener_match_path(Listener *listener, bstring out_path)
{
    bstring path = Request_get(listener->parser, &HTTP_PATH);
    check(path, "Invalid HTTP request, no path (that's unpossible).");

    bassign(out_path, path); // always assign this

    // TODO: find the host from reverse of host name, match it or use default host
   
    // TODO: convert this to actually return all found Backends, then iterate through them all
     
    Backend *found = Host_match(listener->server->default_host, path);

    return found;

error:
    return NULL;
}

int Listener_process_http(Listener *listener)
{
    int rc = 0;
    bstring path = bfromcstr("");
    Backend *found = NULL;

    // TODO: resolve what the default host means, maybe we're matching hosts now?
    check(listener->server->default_host, "No default host set.");

    taskname("Listener_task");

    found = Listener_match_path(listener, path);
    
    if(found) {
        // we can share the data because the caller will block as the proxy runs
        switch(found->type) {
            case BACKEND_PROXY:
                taskstate("proxying");
                rc = Proxy_connect(found->target.proxy, listener->fd, listener->buf, BUFFER_SIZE, listener->nread);
                break;

            case BACKEND_HANDLER:
                taskstate("error");
                sentinel("Handler isn't supported for HTTP yet.");
                break;

            case BACKEND_DIR:
                taskstate("sending");
                rc = Dir_serve_file(found->target.dir, path, listener->fd);
                break;

            default:
                sentinel("Unknown handler type id: %d", found->type);
                break;
        }
    } else {
        // TODO: make a generic error response system
        taskstate("404");

        fdwrite(listener->fd, bdata(&HTTP_404), blength(&HTTP_404));

        taskstate("closing");
        close(listener->fd);

        sentinel("[%s] 404 Not Found", bdata(path));
    }
  
    bdestroy(path);
    return rc;

error:
    bdestroy(path);
    return -1;
}

int Listener_process_flash_socket(Listener *listener)
{
    int rc = Listener_write_func(listener->fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
    check(rc > 0, "Failed to write Flash socket response.");

    return 0;
error:
    return -1;
}

int Listener_parse(Listener *listener)
{
    Request_start(listener->parser);

    int finished = Request_parse(listener->parser, listener->buf,
                        listener->nread, &listener->nparsed);

    check(finished == 1, "Error in parsing: %d, bytes: %d, value: %.*s", 
            finished, listener->nread, listener->nread, listener->buf);

    return 0;
    
error:

    return -1;
}


void Listener_task(void *v)
{
    Listener *listener = (Listener *)v;
    int rc = 0;

    while((listener->nread = Listener_read_func(listener->fd, listener->buf, BUFFER_SIZE-1)) > 0) {
        listener->buf[listener->nread] = '\0';

        rc = Listener_parse(listener);

        // Request_dump(listener->parser);

        check(rc == 0, "Parsing failed, closing %s:%d 'cause they suck.",
                listener->remote, listener->rport);
       
        if(listener->parser->socket_started) {
            rc = Listener_process_flash_socket(listener);
            check(rc == 0, "Invalid flash socket, closing %s:%d 'cause flash sucks.",
                    listener->remote, listener->rport);
            break;
        } else if(listener->parser->json_sent) {
            rc = Listener_process_json(listener);
            check(rc == 0, "Invalid json request, closing %s:%d 'cause they can't read.",
                    listener->remote, listener->rport);
        } else {
            rc = Listener_process_http(listener);
            check(rc == 0, "HTTP hand off failed, closing %s:%d",
                    listener->remote, listener->rport);
            break;
        }
    }

error: // fallthrough for both error or not
    if(listener->parser->json_sent) {
        Register_disconnect(listener->fd);
    }

    Listener_destroy(listener);
}


