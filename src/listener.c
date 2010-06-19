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

void Listener_accept(Handler *base, Proxy *proxy, int fd, const char *remote)
{
    Handler *handler = calloc(sizeof(Handler), 1);
    *handler = *base;

    Listener *listener = Listener_create(handler, proxy, fd, remote);
    taskcreate(Listener_task, listener, LISTENER_STACK);
}


Listener *Listener_create(Handler *handler, Proxy *proxy, int fd, const char *remote)
{
    Listener *listener = calloc(sizeof(Listener), 1);
    check(listener, "Out of memory.");

    listener->handler = handler;
    listener->proxy = proxy;
    listener->fd = fd;

    listener->remote = strdup(remote);
    check(listener->remote, "Out of memory.");


    return listener;
error:
    Listener_destroy(listener);
    return NULL;
}

void Listener_destroy(Listener *listener)
{
    if(listener) {
        // TODO: determine who owns the members
        free(listener->remote);
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

    b64_buf = calloc(1024, 1);
    check(b64_buf, "Failed to allocate buffer for Base64 convert.");

    b64_len = b64_encode(buffer, len, b64_buf, 1024-1);
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



void Listener_task(void *v)
{
    Listener *listener = (Listener *)v;
    Handler *handler = listener->handler;
    int fd = listener->fd;
    char *buf = NULL;
    http_parser *parser = NULL;
    int n = 0;
    int nparsed = 0;
    int finished = 0;
    int registered = 0;
    int rc = 0;

    buf = calloc(1024, 1);
    check(buf, "Failed to allocate parse buffer.");

    parser = calloc(sizeof(http_parser), 1);
    check(parser, "Failed to allocate http_parser.");

    while((n = fdread(fd, buf, 1023)) > 0) {
        buf[n] = '\0';

        http_parser_init(parser);

        nparsed = http_parser_execute(parser, buf, n, 0);
        finished =  http_parser_finish(parser);
        check(finished == 1, "Error in parsing: %d, bytes: %d, value: %.*s", finished, n, n, buf);
        
        if(parser->socket_started) {
            rc = fdwrite(fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
            check(rc > 0, "Failed to write Flash socket response.");
            break;
        } else if(parser->json_sent) {
            if(!registered) {
                Register_connect(fd);
                registered = 1;
            }

            if(strcmp(buf, "{\"type\":\"ping\"}") == 0) {
                if(!Register_ping(fd)) {
                    Register_disconnect(fd);
                    Handler_notify_leave(handler->send_socket, fd);
                }
            } else {
                debug("JSON message sent on jssocket: %.*s", n, buf);

                if(Handler_deliver(handler->send_socket, fd, buf, n) == -1) {
                    log_err("Can't deliver message to handler.");
                }
            }
        } else {
            Handler_destroy(handler, fd);
            free(parser);
            Proxy_connect(listener->proxy, fd, buf, 1024, n);
            return;
        }
    }

error: // fallthrough for both error or not
    if(buf) free(buf);

    Handler_destroy(handler, fd);  // TODO: resolve who should do this
    Listener_destroy(listener);

    if(parser) { 
        if(parser->json_sent) {
            Register_disconnect(fd);
        }
        free(parser);
    }
}


