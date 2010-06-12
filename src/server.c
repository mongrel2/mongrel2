#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task/task.h>
#include <http11/http11_parser.h>
#include <adt/tst.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <zmq.h>
#include <b64/b64.h>

char *FLASH_RESPONSE = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

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

tst_t *registrations = NULL;

void httptask(void*);
void mqtask(void *v);

void taskmain(int argc, char **argv)
{
	int cfd, fd;
    int rport;
	char remote[16];
	
	if(argc != 2){
		fprintf(stderr, "usage: server localport\n");
		taskexitall(1);
	}
	int port = atoi(argv[1]);

	if((fd = netannounce(TCP, 0, port)) < 0){
		fprintf(stderr, "cannot announce on tcp port %d: %s\n", atoi(argv[1]), strerror(errno));
		taskexitall(1);
	}

    fprintf(stderr, "starting mqtask\n");
    taskcreate(mqtask, NULL, STACK);

	fdnoblock(fd);
    fprintf(stderr, "waiting on %d\n", port);
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		taskcreate(httptask, (void*)cfd, STACK);
	}
}


void our_free(void *data, void *hint)
{
    free(data);
}

void mqtask(void *v)
{
    void *socket = mqsocket(ZMQ_REP);
    zmq_msg_t inmsg, outmsg;


    fprintf(stderr, "binding MQ\n");
    if(zmq_bind(socket, "tcp://127.0.0.1:9999") == -1) {
        fprintf(stderr, "oops can't bind: %s\n", strerror(errno));
        taskexitall(1);
    }

    while(1) {
        zmq_msg_init(&inmsg);

        if(mqrecv(socket, &inmsg, 0) == -1) {
            fprintf(stderr, "oops can't read: %s\n", strerror(errno));
            taskexitall(1);
        }

        zmq_msg_close(&inmsg);

        zmq_msg_init_data(&outmsg, strdup("TEST"), strlen("TEST")+1, our_free, NULL);

        if(mqsend(socket, &outmsg, 0) == -1) {
            fprintf(stderr, "oops can't send: %s\n", strerror(errno));
            taskexitall(1);
        }
    }
}


void register_user(char *buffer, int fd)
{
    int len = sprintf(buffer, "%d", fd);

    fprintf(stderr, "registering %s\n", buffer);

    registrations = tst_insert(registrations, buffer, len, (void *)fd);
}

void deliver_message(char *buffer, size_t len)
{
    size_t b64_len = 0;
    b64_char_t b64_buf[512];

    b64_len = b64_encode(buffer, len, b64_buf, sizeof(b64_buf)-1);
    b64_buf[b64_len] = '\0';

    void walker_texas_ranger(void *fd_val, void *buf_data) {
        int to_fd = (int)fd_val;
        char *buffer = (char *)buf_data;

        fdwrite(to_fd, buffer, b64_len+1); 
    }

    fprintf(stderr, "encoded message of len %d: %.*s\n", b64_len, b64_len, b64_buf);
    
    tst_traverse(registrations, walker_texas_ranger, b64_buf);
}

void httptask(void *v)
{
	int fd = (int)v;
    char buf[1024] = {0};
    http_parser parser;
    size_t n = 0;
    size_t nparsed = 0;
    int finished = 0;

	while((n = fdread(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n+1] = '\0';

        http_parser_init(&parser);

        nparsed = http_parser_execute(&parser, buf, n, 0);
        finished =  http_parser_finish(&parser);

        if(finished == -1) {
            fprintf(stderr, "error in parsing\n", n, buf, buf[n]);
            break;
        } else if(finished == 1) {
            if(parser.socket_started) {
                fdwrite(fd, FLASH_RESPONSE, strlen(FLASH_RESPONSE) + 1);
                break;
            } else if(parser.json_sent) {
                fprintf(stderr, "json message sent: %.*s\n", n, buf);

                if(strcmp(buf, "{\"type\":\"join\"}") == 0) {
                    register_user(buf, fd);
                } else {
                    deliver_message(buf, n-1);
                }
            } else {
                fdwrite(fd, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
                break;
            }
        } else {
            fprintf(stderr, "not finished, must implement partial parsing: %.*s\n", n, buf);
            break; 
        }
    }

	shutdown(fd, SHUT_WR);
	close(fd);
}

