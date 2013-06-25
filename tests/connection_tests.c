#include "minunit.h"
#include <connection.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "zmq_compat.h"
#include <task/task.h>
#include <dir.h>

Server *SRV = NULL;

char *test_Connection_create_destroy()
{
    const char remote[IPADDR_SIZE];

    Connection *conn = Connection_create(NULL, 0, 0, remote);
    mu_assert(conn != NULL, "Failed to create a connection.");
    Connection_destroy(conn);

    return NULL;
}

int test_task_with_sample(const char *sample_file)
{
    (void)sample_file;

    check(SRV, "Server isn't configured.");

    Connection *conn = Connection_create(SRV, 12, 1400, "127.0.0.1");
    check(conn, "Failed to create the conn.");

    sentinel("REWRITE NEEDED");

    Connection_task(conn);

    return 1;
error:
    return 0;
}

char *test_Connection_task()
{
    // TODO: bring these back with a different mechanism
    // debug(">>>>> XML <<<<<");
    // mu_assert(test_task_with_sample("tests/sample.xml"), "xml failed.");
    // debug(">>>>> JSON <<<<<");
    // mu_assert(test_task_with_sample("tests/sample.json"), "json failed.");
    // debug(">>>>> GARBAGE <<<<<");
    // mu_assert(test_task_with_sample("tests/sample.garbage"), "garbage failed");
    // debug(">>>>>> HTTP <<<<<<");
    // mu_assert(test_task_with_sample("tests/sample.http"), "http failed");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    Server_init();
    Server *SRV = Server_create(
            bfromcstr("uuid"),
            bfromcstr("localhost"),
            bfromcstr("0.0.0.0"),
            1999,
            bfromcstr("chroot"),
            bfromcstr("access_log"),
            bfromcstr("error_log"),
            bfromcstr("pid_file"),
            NULL,
            0);

    Host *zedshaw_com = Host_create(bfromcstr("zedshaw.com"), bfromcstr("zedshaw.com"));

    Host_add_backend(zedshaw_com, bfromcstr("@chat"), BACKEND_HANDLER, NULL);

    Dir *tests = Dir_create(
            bfromcstr("tests/"),
            bfromcstr("index.html"),
            bfromcstr("text/plain"),
            0);

    Host_add_backend(zedshaw_com, bfromcstr("/tests"), BACKEND_DIR, tests);

    Server_set_default_host(SRV, zedshaw_com);

    mu_run_test(test_Connection_create_destroy);
    mu_run_test(test_Connection_task);

    Server_destroy(SRV);
    // TODO: the above will eventually do this
    Host_destroy(zedshaw_com);
    zmq_term(ZMQ_CTX);
    return NULL;
}

RUN_TESTS(all_tests);
