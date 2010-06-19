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
    SocketPair *pair = (SocketPair *)v;
	int fd = pair->fd;
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
        check(finished == 1, "Error in parsing: %d, %.*s", finished, n, buf);
        
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
                    Handler_notify_leave(pair->handler, fd);
                }
            } else {
                debug("JSON message sent on jssocket: %.*s", n, buf);

                if(Handler_deliver(pair->handler, fd, buf, n) == -1) {
                    log_err("Can't deliver message to handler.");
                }
            }
        } else {
            // HTTP, proxy it back
            ProxyConnect *conn = ProxyConnect_create(fd, buf, 1024, n); 
            Proxy_connect(conn);
            free(pair);
            free(parser);
            return;
        }
    }

error: // fallthrough for both error or not
    if(buf) free(buf);
    if(pair) {
        Handler_notify_leave(pair->handler, fd);
        free(pair);
    }

    if(parser) { 
        if(parser->json_sent) {
            Register_disconnect(fd);
        }
        free(parser);
    }
}


void *Listener_create(const char *listener_spec, const char *uuid)
{
    void *listener_socket = mqsocket(ZMQ_SUB);
    check(listener_socket, "Can't create ZMQ_SUB socket.");

    int rc = zmq_setsockopt(listener_socket, ZMQ_SUBSCRIBE, uuid, 0);
    check(rc == 0, "Failed to subscribe listener socket: %s", listener_spec);
    debug("binding listener SUB socket %s subscribed to: %s", listener_spec, uuid);

    rc = zmq_bind(listener_socket, listener_spec);
    check(rc == 0, "Can't bind listener socket %s", listener_spec);

    FLASH_LEN = strlen(FLASH_RESPONSE);

    return listener_socket;

error:
    return NULL;
}


