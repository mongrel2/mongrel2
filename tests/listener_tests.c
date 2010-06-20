#include "minunit.h"
#include <listener.h>
#include <string.h>

FILE *LOG_FILE = NULL;

char *test_Listener_init() 
{
    mqinit(1);

    Listener_init();

    return NULL;
}

char *test_Listener_create_destroy()
{
    Proxy *proxy = Proxy_create("127.0.0.1", 80);
    mu_assert(proxy != NULL, "Failed to make proxy.");

    Handler *handler = Handler_create("tcp://127.0.0.1:1234", "ZED", "tcp://127.0.0.1:4321", "ZED");
    mu_assert(handler != NULL, "Failed to make the handler.");

    Listener *listener = Listener_create(handler, proxy, 12, "127.0.0.1"); 
    mu_assert(listener != NULL, "Failed to make listener.");

    Handler_destroy(handler, 12);
    Proxy_destroy(proxy);
    Listener_destroy(listener);

    return NULL;
}


char *test_Listener_deliver()
{
    int rc = Listener_deliver(1, "TEST", strlen("TEST"));
    printf("\n");
    mu_assert(rc == 0, "Failed to write to stderr.");

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Listener_init);
    mu_run_test(test_Listener_create_destroy);
    mu_run_test(test_Listener_deliver);

    return NULL;
}

RUN_TESTS(all_tests);

