#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task/task.h>
#include <http11/http11_parser.h>
#include <adt/tst.h>
#include <adt/hash.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <zmq.h>
#include <b64/b64.h>
#include <assert.h>
#include <dbg.h>
#include <proxy.h>


char *FLASH_RESPONSE = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

FILE *LOG_FILE = NULL;

size_t FLASH_LEN = 0;

static char *LEAVE_MSG = "{\"type\":\"leave\"}";
size_t LEAVE_MSG_LEN = 0;

char *HTTP_RESPONSE = "HTTP/1.1 200 OK\r\n"
    "Server: mongrel2/0.1\r\n"
    "Date: Tue, 08 Jun 2010 04:33:23 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 2\r\n"
    "Last-Modified: Tue, 08 Jun 2010 04:00:07 GMT\r\n"
    "Connection: close\r\n\r\nHI";

char *UUID = "907F620B-BC91-4C93-86EF-512B71C2AE27";

enum
{
	LISTENER_STACK = 32 * 1024,
    HANDLER_STACK = 100 * 1024

};



void from_listener_task(void*);
void from_handler_task(void *v);
void register_connect(int fd);
void unregister_connect(int fd, int announce_leave);
void ping_connect(int fd);
int listener_deliver(int to_fd, char *buffer, size_t len);
int handler_deliver(int from_fd, char *buffer, size_t len);


void *handler_socket = NULL;
void *listener_socket = NULL;
hash_t *registrations = NULL;


hash_val_t fd_hash_func(const void *fd)
{
    return (hash_val_t)fd;
}

int fd_comp_func(const void *a, const void *b)
{
    return a - b;
}




void our_free(void *data, void *hint)
{
    free(data);
}


void from_handler_task(void *socket)
{
    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    char *data = NULL;
    size_t sz = 0;
    int fd = 0;
    int rc = 0;

    while(1) {
        zmq_msg_init(inmsg);

        rc = mqrecv(socket, inmsg, 0);
        check(rc == 0, "Receive on handler socket failed.");

        data = (char *)zmq_msg_data(inmsg);
        sz = zmq_msg_size(inmsg);

        if(data[sz-1] != '\0') {
            log_err("Last char from handler is not 0 it's %d, fix your backend.", data[sz-1]);
        } if(data[sz-2] == '\0') {
            log_err("You have two \0 ending your message, that's bad.");
        } else {
            int end = 0;
            int ok = sscanf(data, "%u%n", &fd, &end);
            debug("MESSAGE from handler: %s for fd: %d, nread: %d, len: %d, final: %d, last: %d",
                    data, fd, end, sz, sz-end-1, (data + end)[sz-end-1]);

            if(ok <= 0 || end <= 0) {
                log_err("Message didn't start with a ident number.");
            } else if(!hash_lookup(registrations, (void *)fd)) {
                log_err("Ident %d is no longer connected.", fd);

                if(handler_deliver(fd, LEAVE_MSG, LEAVE_MSG_LEN) == -1) {
                    log_err("Can't tell handler %d died.", fd);
                }
            } else {
                if(listener_deliver(fd, data+end, sz-end-1) == -1) {
                    log_err("Error sending to listener %d, closing them.");
                    unregister_connect(fd, 1);
                }
            }
        }
    }

    return;
error:
    taskexitall(1);
}


void register_connect(int fd)
{
    debug("Registering %d ident.", fd);

    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) hash_delete_free(registrations, hn);

    check(hash_alloc_insert(registrations, (void *)fd, NULL), "Cannot register fd, out of space.");

    debug("Currently registered idents: %d", hash_count(registrations));

    return;

error:
    taskexitall(1);
}

void unregister_connect(int fd, int announce_leave)
{
    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) {
        debug("Unregistering %d", fd);

        hash_delete_free(registrations, hn);

        if(close(fd) == -1) {
            log_err("Failed on close for ident %d, not sure why.", fd);
        }

        if(announce_leave) {
            if(handler_deliver(fd, LEAVE_MSG, LEAVE_MSG_LEN) == -1) {
                log_err("Can't deliver unregister to handler.");
            }
        }
    } else {
        log_err("Ident %d was unregistered but doesn't exist in registrations.", fd);
    }
}

void ping_connect(int fd)
{
    debug("Ping received for ident %d", fd);
    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    check(hn, "Ping received but %d isn't actually registerd.", fd);
    // TODO: increment its ping time
    
    return;

error:
    taskexitall(1);
}


int listener_deliver(int to_fd, char *buffer, size_t len)
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


int handler_deliver(int from_fd, char *buffer, size_t len)
{
    int rc = 0;
    zmq_msg_t *msg = NULL;
    char *msg_buf = NULL;
    int msg_size = 0;
    size_t sz = 0;

    msg = calloc(sizeof(zmq_msg_t), 1);
    msg_buf = NULL;

    check(msg, "Failed to allocate 0mq message to send.");

    rc = zmq_msg_init(msg);
    check(rc == 0, "Failed to initialize 0mq message to send.");

    sz = strlen(buffer) + 32;
    msg_buf = malloc(sz);
    check(msg_buf, "Failed to allocate message buffer for handler delivery.");

    msg_size = snprintf(msg_buf, sz, "%d %.*s", from_fd, len, buffer);
    check(msg_size > 0, "Message too large, killing it.");

    rc = zmq_msg_init_data(msg, msg_buf, msg_size, our_free, NULL);
    check(rc == 0, "Failed to init 0mq message data.");

    rc = zmq_send(handler_socket, msg, 0);
    check(rc == 0, "Failed to deliver 0mq message to handler.");

    if(msg) free(msg);
    return 0;

error:
    if(msg) free(msg);
    if(msg_buf) free(msg_buf);
    return -1;
}


void from_listener_task(void *v)
{
	int fd = (int)v;
    char *buf = NULL;
    http_parser *parser = NULL;
    size_t n = 0;
    size_t nparsed = 0;
    int finished = 0;
    int registered = 0;
    int rc = 0;

    buf = calloc(1024, 1);
    check(buf, "Failed to allocate parse buffer.");

    parser = calloc(sizeof(http_parser), 1);
    check(parser, "Failed to allocate http_parser.");

	while((n = fdread(fd, buf, 1024)) > 0) {
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
                register_connect(fd);
                registered = 1;
            }

            if(strcmp(buf, "{\"type\":\"ping\"}") == 0) {
                ping_connect(fd);
            } else {
                debug("JSON message sent on jssocket: %.*s", n, buf);

                if(handler_deliver(fd, buf, n) == -1) {
                    log_err("Can't deliver message to handler.");
                }
            }
        } else {
            // HTTP, proxy it back
            ProxyConnect *conn = ProxyConnect_create(fd, buf, 1024, n); 
            proxy_connect(conn);

            // set buf to NULL so that we don't close it, the proxy owns it
            buf = NULL;
            break;
        }
    }

error: // fallthrough for both error or not
    if(buf) free(buf);
    if(parser) free(parser);
    if(parser->json_sent) {
        unregister_connect(fd, 1);
    }
}


void taskmain(int argc, char **argv)
{
	int cfd, fd;
    int rport;
	char remote[16];
    FLASH_LEN = strlen(FLASH_RESPONSE);
    LEAVE_MSG_LEN = strlen(LEAVE_MSG);
    int rc = 0;
    LOG_FILE = stderr;
	
    check(argc == 4, "usage: server localport handlerq listenerq");
    char *handler_spec = argv[2];
    char *listener_spec = argv[3];

    mqinit(2);
    proxy_init("127.0.0.1", 8080);

    registrations = hash_create(HASHCOUNT_T_MAX, fd_comp_func, fd_hash_func);
    check(registrations, "Failed creating registrations store.");

	int port = atoi(argv[1]);
    check(port > 0, "Can't bind to the given port: %s", argv[1]);

    handler_socket = mqsocket(ZMQ_PUB);
    rc = zmq_setsockopt(handler_socket, ZMQ_IDENTITY, UUID, strlen(UUID));
    check(rc == 0, "Failed to set handler socket %s identity %s", handler_spec, UUID);

    debug("Binding handler PUB socket %s with identity: %s", argv[2], UUID);

    rc = zmq_bind(handler_socket, handler_spec);
    check(rc == 0, "Can't bind handler socket: %s", handler_spec);

    listener_socket = mqsocket(ZMQ_SUB);
    rc = zmq_setsockopt(listener_socket, ZMQ_SUBSCRIBE, "", 0);
    check(rc == 0, "Failed to subscribe listener socket: %s", listener_spec);
    debug("binding listener SUB socket %s subscribed to: %s", listener_spec, UUID);

    rc = zmq_bind(listener_socket, listener_spec);
    check(rc == 0, "Can't bind listener socket %s", listener_spec);

	fd = netannounce(TCP, 0, port);
    check(fd >= 0, "Can't announce on TCP port %d", port);

    debug("Starting server on port %d", port);
    taskcreate(from_handler_task, listener_socket, HANDLER_STACK);

	fdnoblock(fd);
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		taskcreate(from_listener_task, (void*)cfd, LISTENER_STACK);
	}



error:
    log_err("Exiting due to error.");
    taskexitall(1);
}
