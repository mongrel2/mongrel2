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

char *HTTP_RESPONSE = "HTTP/1.1 200 OK\r\n"
    "Server: mongrel2/0.1\r\n"
    "Date: Tue, 08 Jun 2010 04:33:23 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 2\r\n"
    "Last-Modified: Tue, 08 Jun 2010 04:00:07 GMT\r\n"
    "Connection: close\r\n\r\nHI";

enum
{
	STACK = 32 * 1024
};


void listener_task(void*);
void handler_task(void *v);
void register_connect(int fd);
void unregister_connect(int fd);
void ping_connect(int fd);
void listener_deliver(int to_fd, char *buffer, size_t len);


void *handler_socket = NULL;
void *listener_socket = NULL;


void taskmain(int argc, char **argv)
{
	int cfd, fd;
    int rport;
	char remote[16];
    FLASH_LEN = strlen(FLASH_RESPONSE);
	
	if(argc != 3){
		fprintf(stderr, "usage: server localport handlerq listenerq\n");
		taskexitall(1);
	}

	int port = atoi(argv[1]);

    handler_socket = mqsocket(ZMQ_PUB);
    fprintf(stderr, "binding handler\n");
    if(zmq_bind(socket, argv[2]) == -1) {
        fprintf(stderr, "error can't bind %s: %s\n", argv[2], strerror(errno));
        taskexitall(1);
    }

    listener_socket = mqsocket(ZMQ_SUB);
    if(zmq_bind(socket, argv[3]) == -1) {
        fprintf(stderr, "error can't bind %s: %s\n", argv[3], strerror(errno));
        taskexitall(1);
    }

	if((fd = netannounce(TCP, 0, port)) < 0){
		fprintf(stderr, "cannot announce on tcp port %d: %s\n", atoi(argv[1]), strerror(errno));
		taskexitall(1);
	}

    fprintf(stderr, "starting handler_task\n");
    taskcreate(handler_task, NULL, STACK);

	fdnoblock(fd);
    fprintf(stderr, "waiting on %d\n", port);
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		taskcreate(listener_task, (void*)cfd, STACK);
	}
}


void our_free(void *data, void *hint)
{
    free(data);
}


void handler_task(void *v)
{
    zmq_msg_t inmsg;
    int fd = 0;

    while(1) {
        zmq_msg_init(&inmsg);

        if(mqrecv(listener_socket, &inmsg, 0) == -1) {
            fprintf(stderr, "oops can't read: %s\n", strerror(errno));
            taskexitall(1);
        }

        // lookup the fd
        listener_deliver(fd, (char *)zmq_msg_data(&inmsg), zmq_msg_size(&inmsg));
    }
}


void register_connect(int fd)
{

}

void unregister_connect(int fd)
{
	shutdown(fd, SHUT_WR);
	close(fd);
}

void ping_connect(int fd)
{

}


void listener_deliver(int to_fd, char *buffer, size_t len)
{
    size_t b64_len = 0;
    b64_char_t b64_buf[512] = {0};
    assert(buffer[len] == '\0' && "invalid message, must end in \0");

    b64_len = b64_encode(buffer, len-1, b64_buf, sizeof(b64_buf)-1);
    b64_buf[b64_len] = '\0';

    fdwrite(to_fd, buffer, b64_len+1); 
}


void handler_deliver(int from_fd, char *buffer, size_t len)
{



}


void listener_task(void *v)
{
	int fd = (int)v;
    char *buf = calloc(1024, 1);
    http_parser parser;
    size_t n = 0;
    size_t nparsed = 0;
    int finished = 0;
    int registered = 0;

	while((n = fdread(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n+1] = '\0';

        http_parser_init(&parser);

        nparsed = http_parser_execute(&parser, buf, n, 0);
        finished =  http_parser_finish(&parser);

        if(finished != 1) {
            fprintf(stderr, "error in parsing: %d\n", finished);
            break;
        }
        
        if(parser.socket_started) {
            fdwrite(fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
            break;
        } else if(parser.json_sent) {
            if(!registered) {
                register_connect(fd);
            }

            if(strcmp(buf, "{\"type\":\"ping\"}") == 0) {
                ping_connect(fd);
            } else {
                fprintf(stderr, "json message sent: %.*s\n", n, buf);
                listener_deliver(fd, buf, n);
            }
        } else {
            fdwrite(fd, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
            break;
        }
    }

    unregister_connect(fd);
    free(buf);
}

