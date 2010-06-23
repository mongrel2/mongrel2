#include <listener.h>
#include <b64/b64.h>
#include <http11/http11_parser.h>
#include <string.h>
#include <proxy.h>
#include <zmq.h>
#include <task/task.h>
#include <dbg.h>
#include <handler.h>


char *FLASH_RESPONSE = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

size_t FLASH_LEN = 0;

void Listener_init()
{
    FLASH_LEN = strlen(FLASH_RESPONSE);
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
    strncpy(listener->remote, remote, IPADDR_SIZE);
    listener->remote[IPADDR_SIZE] = '\0';

    listener->parser = calloc(sizeof(http_parser), 1);
    check(listener->parser, "Failed to allocate http_parser.");

    return listener;
error:
    Listener_destroy(listener);
    return NULL;
}

void Listener_destroy(Listener *listener)
{
    if(listener) {
        free(listener->parser);
        free(listener);
    }
}


int Listener_deliver(int to_fd, char *buffer, size_t len)
{
    size_t b64_len = 0;
    b64_char_t *b64_buf = NULL;
    int rc = 0;

    check(buffer[len] == '\0', "Message for listener must end in \\0, you have '%c'", buffer[len]);
    check(buffer[len-1] != '\0', "Message for listener must end in ONE \\0, you have more.");

    b64_buf = calloc(BUFFER_SIZE * 1.5, 1);
    check(b64_buf, "Failed to allocate buffer for Base64 convert.");

    b64_len = b64_encode(buffer, len, b64_buf, BUFFER_SIZE * 1.5-1);
    check(b64_len > 0, "Base64 convert failed.");

    b64_buf[b64_len] = '\0';

    rc = fdwrite(to_fd, b64_buf, b64_len+1);
    check(rc == b64_len+1, "Failed to write entire message to listener %d", to_fd);


    if(b64_buf) free(b64_buf);
    return 0;

error:
    if(b64_buf) free(b64_buf);
    return -1;
}



int Listener_process_json(Listener *listener)
{
    check(listener->server->default_host, "No default host set, need one for jssockets to work.");

    if(!listener->registered) {
        Register_connect(listener->fd);
        listener->registered = 1;
    }

    // TODO: strcmp is lame
    if(strcmp(listener->buf, "{\"type\":\"ping\"}") == 0) {
        if(!Register_ping(listener->fd)) {
            Register_disconnect(listener->fd);
        }

    } else {
        // TODO: actually, like, parse the route yo
        Backend *found = Host_match(listener->server->default_host, "@chat", strlen("@chat"));
        check(found, "Didn't find a route named @chat, nowhere to go.");
        check(found->type == BACKEND_HANDLER, "@chat route should be handler type.");

        debug("JSON message from %s:%d sent on jssocket: %.*s", listener->remote, listener->fd, listener->nread, listener->buf);
        check(found->target.handler, "Oops, handler got reset, how'd that happen?.");

        if(Handler_deliver(found->target.handler->send_socket, listener->fd, listener->buf, listener->nread) == -1) {
            log_err("Can't deliver message to handler.");
        }
    }

    return 0;
error:
    return -1;
}


int Listener_process_http(Listener *listener)
{
    // TODO: get the right path and host from the http request
    check(listener->server->default_host, "No default host set.");

    Backend *found = Host_match(listener->server->default_host, "/chat/", strlen("/chat/"));
    check(found, "Didn't find a route named /chat/, nowhere to go.");
    check(found->type == BACKEND_PROXY, "/chat/ route should be proxy type.");

    // we can share the data because the caller will block as the proxy runs
    Proxy_connect(found->target.proxy, listener->fd, listener->buf, BUFFER_SIZE, listener->nread);
   
    return 1;
error:
    return -1;
}

int Listener_process_flash_socket(Listener *listener)
{
    int rc = fdwrite(listener->fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
    check(rc > 0, "Failed to write Flash socket response.");

    return 0;
error:
    return -1;
}

int Listener_parse(Listener *listener)
{
    http_parser_init(listener->parser);

    listener->nparsed = http_parser_execute(listener->parser, listener->buf, listener->nread, 0);
    listener->finished =  http_parser_finish(listener->parser);
    check(listener->finished == 1, "Error in parsing: %d, bytes: %d, value: %.*s", 
            listener->finished, listener->nread, listener->nread, listener->buf);

    return 0;
    
error:

    return -1;
}


void Listener_task(void *v)
{
    Listener *listener = (Listener *)v;
    int rc = 0;

    while((listener->nread = fdread(listener->fd, listener->buf, BUFFER_SIZE-1)) > 0) {
        listener->buf[listener->nread] = '\0';

        rc = Listener_parse(listener);
        check(rc == 0, "Parsing failed, closing %s:%d 'cause they suck.",
                listener->remote, listener->fd);
        
        if(listener->parser->socket_started) {
            rc = Listener_process_flash_socket(listener);
            check(rc == 0, "Invalid flash socket, closing %s:%d 'cause flash sucks.",
                    listener->remote, listener->fd);
            break;
        } else if(listener->parser->json_sent) {
            rc = Listener_process_json(listener);
            check(rc == 0, "Invalid json request, closing %s:%d 'cause they can't read.",
                    listener->remote, listener->fd);
        } else {
            rc = Listener_process_http(listener);
            check(rc == 0, "HTTP hand off failed, closing %s:%d",
                    listener->remote, listener->fd);
            return;
        }
    }

error: // fallthrough for both error or not
    if(listener->parser->json_sent) {
        Register_disconnect(listener->fd);
    }

    Listener_destroy(listener);
}


