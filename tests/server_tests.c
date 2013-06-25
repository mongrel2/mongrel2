#include "minunit.h"
#include <server.h>
#include <string.h>
#include <task/task.h>

char *test_Server_init() 
{
    mqinit(1);
    Server_init();

    return NULL;
}


char *test_Server_create_destroy()
{
    Server *server = Server_create(
            bfromcstr("uuid"),
            bfromcstr("localhost"),
            bfromcstr("0.0.0.0"),
            8080,
            bfromcstr("chroot"),
            bfromcstr("access_log"),
            bfromcstr("error_log"),
            bfromcstr("pid_file"),
            NULL,
            0);
    mu_assert(server != NULL, "Failed to make the server, something on 8090?");

    Server_destroy(server);

    return NULL;
}


char *test_Server_adds()
{
    int rc = 0;

    Server *srv = Server_create(
            bfromcstr("uuid"),
            bfromcstr("localhost"),
            bfromcstr("0.0.0.0"),
            8080,
            bfromcstr("chroot"),
            bfromcstr("access_log"),
            bfromcstr("error_log"),
            bfromcstr("pid_file"),
            NULL,
            0);
    mu_assert(srv != NULL, "Failed to make the server, something on 8090?");

    Host *host = Host_create(bfromcstr("zedshaw.com"), bfromcstr("zedshaw.com"));
    mu_assert(host != NULL, "Failed to make host.");

    rc = Server_add_host(srv, host);
    mu_assert(rc == 0, "Failed to add host to server.");

    Server_set_default_host(srv, host);

    Host *zedshaw = Server_match_backend(srv, host->name);
    mu_assert(zedshaw == host, "Didn't get the right one back.");

    mu_assert(Server_match_backend(srv, bfromcstr("NOWAY")) == host, "Didn't fall back to default_host");

    Server_destroy(srv);

    return NULL;
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_Server_init);
    mu_run_test(test_Server_create_destroy);
    mu_run_test(test_Server_adds);
    zmq_term(ZMQ_CTX);

    return NULL;
}

RUN_TESTS(all_tests);

