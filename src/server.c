#include <dbg.h>
#include <task/task.h>
#include <proxy.h>
#include <listener.h>
#include <register.h>
#include <handler.h>
#include <server.h>


FILE *LOG_FILE = NULL;

char *UUID = "907F620B-BC91-4C93-86EF-512B71C2AE27";

typedef struct Server {
    

} Server;

void taskmain(int argc, char **argv)
{
    int cfd, fd;
    int rport;
    char remote[16];
    int rc = 0;
    LOG_FILE = stderr;
    Handler *handler = NULL;
    Proxy *proxy = NULL;

    check(argc == 4, "usage: server localport handlerq listenerq");
    char *send_spec = argv[2];
    char *recv_spec = argv[3];

    mqinit(2);
    Register_init();
    Listener_init();
    Handler_init();

    proxy = Proxy_create("127.0.0.1", 80);
    check(proxy, "Failed to create proxy configuration.");

    int port = atoi(argv[1]);
    check(port > 0, "Can't bind to the given port: %s", argv[1]);

    handler = Handler_create(send_spec, UUID, recv_spec, "");
    check(handler, "Failed to create handler for %s/%s UUID %s", send_spec, recv_spec, UUID);

    fd = netannounce(TCP, 0, port);
    check(fd >= 0, "Can't announce on TCP port %d", port);

    debug("Starting server on port %d", port);
    taskcreate(Handler_task, handler, HANDLER_STACK);

    check(fdnoblock(fd) == 0, "Failed to set listening port %d nonblocking.", port);

    while((cfd = netaccept(fd, remote, &rport)) >= 0) {
        Listener_accept(handler, proxy, cfd, remote);
    }

    // TODO: handle sigint and make a service port

    return;

error:
    log_err("Exiting due to error.");
    taskexitall(1);
}

