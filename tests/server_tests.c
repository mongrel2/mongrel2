#include "minunit.h"
#include <server.h>
#include <string.h>

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

    Host *host = Host_create("zedshaw.com");
    mu_assert(host != NULL, "Failed to make host.");

    rc = Server_add_host(srv, host->name, strlen(host->name), host);
    mu_assert(rc == 0, "Failed to add host to server.");

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

