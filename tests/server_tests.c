#include "minunit.h"
#include <server.h>

FILE *LOG_FILE = NULL;

char *test_Server_init() 
{
    mqinit(1);

    Server_init();

    return NULL;
}


char *test_Server_create_destroy()
{
    Server *server = Server_create("8090");
    mu_assert(server != NULL, "Failed to make the server, something on 8090?");

    Server_destroy(server);

    return NULL;
}


char *test_Server_adds()
{
    int rc = 0;

    Server *srv = Server_create("8090");
    mu_assert(srv != NULL, "Failed to make the server, something on 8090?");

    Proxy *proxy = Proxy_create("127.0.0.1", 80);
    mu_assert(proxy != NULL, "Didn't make the proxy.");

    rc = Server_add_proxy(srv, proxy);
    mu_assert(rc > 0, "Failed to add proxy to server.");


    Handler *handler = Handler_create("tcp://127.0.0.1:1234", "ZED", "tcp://127.0.0.1:4321", "ZED");
    mu_assert(handler != NULL, "Failed to make the handler.");

    rc = Server_add_handler(srv, handler);
    mu_assert(rc > 0, "Failed to add handler to server.");

    Host *host = Host_create("zedshaw.com");
    mu_assert(host != NULL, "Failed to make host.");

    rc = Server_add_host(srv, host);
    mu_assert(rc > 0, "Failed to add host to server.");

    Handler_destroy(handler, 0);
    Proxy_destroy(proxy);
    Host_destroy(host);
    Server_destroy(srv);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Server_init);
    mu_run_test(test_Server_create_destroy);
    mu_run_test(test_Server_adds);

    return NULL;
}

RUN_TESTS(all_tests);

