#include <dbg.h>
#include <task/task.h>
#include <proxy.h>
#include <listener.h>
#include <register.h>
#include <handler.h>


FILE *LOG_FILE = NULL;

char *UUID = "907F620B-BC91-4C93-86EF-512B71C2AE27";



void taskmain(int argc, char **argv)
{
    int cfd, fd;
    int rport;
    char remote[16];
    int rc = 0;
    LOG_FILE = stderr;
    SocketPair *pair = calloc(sizeof(SocketPair), 1);

    check(argc == 4, "usage: server localport handlerq listenerq");
    char *handler_spec = argv[2];
    char *listener_spec = argv[3];

    mqinit(2);
    Register_init();

    Proxy_init("127.0.0.1", 80);

    int port = atoi(argv[1]);
    check(port > 0, "Can't bind to the given port: %s", argv[1]);

    pair->handler = Handler_create(handler_spec, UUID);
    check(pair->handler, "Failed to create handler socket.");

    pair->listener = Listener_create(listener_spec, "");  // TODO: add uuid
    check(pair->listener, "Failed to create listener socket.");

    fd = netannounce(TCP, 0, port);
    check(fd >= 0, "Can't announce on TCP port %d", port);

    debug("Starting server on port %d", port);
    taskcreate(Handler_task, pair, HANDLER_STACK);

    fdnoblock(fd);

    while((cfd = netaccept(fd, remote, &rport)) >= 0) {
        SocketPair *p = calloc(sizeof(SocketPair), 1);
        *p = *pair;
        p->fd = cfd;
        taskcreate(Listener_task, p, LISTENER_STACK);
    }

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

