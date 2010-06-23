#include "minunit.h"
#include <listener.h>
#include <server.h>
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
    Server *srv = Server_create("19999");
    mu_assert(srv != NULL, "Failed to make the server to test with.");

    Listener *listener = Listener_create(srv, 12, 1400, "127.0.0.1"); 
    mu_assert(listener != NULL, "Failed to make listener.");

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

