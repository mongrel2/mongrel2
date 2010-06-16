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

char *FLASH_RESPONSE = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

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
void listener_deliver(int to_fd, char *buffer, size_t len);
void handler_deliver(int from_fd, char *buffer, size_t len);


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


void taskmain(int argc, char **argv)
{
	int cfd, fd;
    int rport;
	char remote[16];
    FLASH_LEN = strlen(FLASH_RESPONSE);
    LEAVE_MSG_LEN = strlen(LEAVE_MSG);
	
	if(argc != 4){
		fprintf(stderr, "usage: server localport handlerq listenerq\n");
		taskexitall(1);
	}

    mqinit(2);

    registrations = hash_create(HASHCOUNT_T_MAX, fd_comp_func, fd_hash_func);
    assert(registrations && "Can't make the registration hash.");

	int port = atoi(argv[1]);

    handler_socket = mqsocket(ZMQ_PUB);
    zmq_setsockopt(handler_socket, ZMQ_IDENTITY, UUID, strlen(UUID));
    fprintf(stderr, "binding handler PUB socket %s with identity: %s\n", argv[2], UUID);

    if(zmq_bind(handler_socket, argv[2]) == -1) {
        fprintf(stderr, "error can't bind %s: %s\n", argv[2], strerror(errno));
        taskexitall(1);
    }

    listener_socket = mqsocket(ZMQ_SUB);
    zmq_setsockopt(listener_socket, ZMQ_SUBSCRIBE, "", 0);

    fprintf(stderr, "binding listener SUB socket %s subscribed to: %s\n", argv[3], UUID);
    if(zmq_bind(listener_socket, argv[3]) == -1) {
        fprintf(stderr, "error can't bind %s: %s\n", argv[3], strerror(errno));
        taskexitall(1);
    }

	if((fd = netannounce(TCP, 0, port)) < 0){
		fprintf(stderr, "cannot announce on tcp port %d: %s\n", atoi(argv[1]), strerror(errno));
		taskexitall(1);
	}

    fprintf(stderr, "starting from_handler_task\n");
    taskcreate(from_handler_task, listener_socket, HANDLER_STACK);

	fdnoblock(fd);
    fprintf(stderr, "waiting on %d\n", port);
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		taskcreate(from_listener_task, (void*)cfd, LISTENER_STACK);
	}
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

    while(1) {
        zmq_msg_init(inmsg);

        fprintf(stderr, "WAIT FROM HANDLER\n");
        if(mqrecv(socket, inmsg, 0) == -1) {
            fprintf(stderr, "oops can't read: %s\n", strerror(errno));
            taskexitall(1);
        }

        data = (char *)zmq_msg_data(inmsg);
        sz = zmq_msg_size(inmsg);
        if(data[sz-1] != '\0') {
            fprintf(stderr, "error, last char from handler is not 0 it's %d\n", data[sz-1]);
        } else {
            int end = 0;
            int ok = sscanf(data, "%u%n", &fd, &end);
            fprintf(stderr, "!!! message from handler: %s for fd: %d, nread: %d, final: %d, last: %d\n",
                    data, fd, end, sz-end-1, (data + end)[sz-end-1]);

            if(ok > 0 && end > 0 && hash_lookup(registrations, (void *)fd)) {
                listener_deliver(fd, data+end, sz-end-1);
            }
        }
    }
}


void register_connect(int fd)
{
    fprintf(stderr, "register for %d\n", fd);

    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) hash_delete_free(registrations, hn);

    if(!hash_alloc_insert(registrations, (void *)fd, NULL)) {
        fprintf(stderr, "cannot register fd");
        return;
    }

    fprintf(stderr, "currently registered: %d\n", hash_count(registrations));
}

void unregister_connect(int fd, int announce_leave)
{
    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) {
        fprintf(stderr, "unregister %d\n", fd);
        hash_delete_free(registrations, hn);

        if(close(fd) == -1) {
            fprintf(stderr, "errno after close: %s\n", strerror(errno));
        }

        if(announce_leave) {
            handler_deliver(fd, LEAVE_MSG, LEAVE_MSG_LEN);
        }
    }

    fprintf(stderr, "currently registered: %d\n", hash_count(registrations));
}

void ping_connect(int fd)
{
    fprintf(stderr, "ping for %d\n", fd);
    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    // TODO: increment its ping time
}


void listener_deliver(int to_fd, char *buffer, size_t len)
{
    size_t b64_len = 0;
    fprintf(stderr, "delivering %.*s to %d\n", len, buffer, to_fd);
    if(buffer[len] != '\0') {
        fprintf(stderr, "must end in NUL\n");
        return;
    } else if (buffer[len-1] == '\0') {
        fprintf(stderr, "must end in only 1 NUL, you have too many\n");
        return;
    }

    b64_char_t *b64_buf = calloc(1024, 1);

    b64_len = b64_encode(buffer, len, b64_buf, 1024-1);
    b64_buf[b64_len] = '\0';

    fprintf(stderr, "b64 is %s\n", b64_buf);

    fdwrite(to_fd, b64_buf, b64_len+1); 
    free(b64_buf);
}


void handler_deliver(int from_fd, char *buffer, size_t len)
{
    zmq_msg_t *msg = calloc(sizeof(zmq_msg_t), 1);

    zmq_msg_init(msg);

    fprintf(stderr, "sending msg to handler backends: %.*s\n", len, buffer);

    size_t sz = strlen(buffer) + 32;
    char *msg_buf = malloc(sz);
    int msg_size = snprintf(msg_buf, sz, "%d %.*s", from_fd, len, buffer);

    if(msg_size > 0) {
        zmq_msg_init_data(msg, msg_buf, msg_size, our_free, NULL);
        zmq_send(handler_socket, msg, 0);
    } else {
        free(msg_buf);
    }

    free(msg);
}


void from_listener_task(void *v)
{
	int fd = (int)v;
    char *buf = calloc(1024, 1);
    http_parser *parser = calloc(sizeof(http_parser), 1);
    size_t n = 0;
    size_t nparsed = 0;
    int finished = 0;
    int registered = 0;

	while((n = fdread(fd, buf, 1024-1)) > 0) {
        buf[n+1] = '\0';

        http_parser_init(parser);

        nparsed = http_parser_execute(parser, buf, n, 0);
        finished =  http_parser_finish(parser);

        if(finished != 1) {
            fprintf(stderr, "error in parsing: %d, %.*s\n", finished, n, buf);
            break;
        }
        
        if(parser->socket_started) {
            fdwrite(fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
            break;
        } else if(parser->json_sent) {
            if(!registered) {
                register_connect(fd);
            }

            if(strcmp(buf, "{\"type\":\"ping\"}") == 0) {
                ping_connect(fd);
            } else {
                fprintf(stderr, "json message sent: %.*s\n", n, buf);
                handler_deliver(fd, buf, n);
            }
        } else {
            fdwrite(fd, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
            break;
        }
    }

    if(parser->json_sent) {
        unregister_connect(fd, 1);
    }

    free(buf);
    free(parser);
}

