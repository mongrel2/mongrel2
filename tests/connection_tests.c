#include "minunit.h"
#include <connection.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <zmq.h>
#include <task/task.h>

FILE *LOG_FILE = NULL;

Server *SRV = NULL;

char *test_Connection_create_destroy() 
{
    const char remote[IPADDR_SIZE];

    Connection *conn = Connection_create(NULL, 0, 0, remote);
    Connection_destroy(conn);

    return NULL;
}

char *test_Connection_deliver()
{
    bstring t1;

    int rc = Connection_deliver(1, t1 = bfromcstr("TEST"));
    // depending on the platform this will fail or not if send is allowed on files
    mu_assert(rc == -1, "Should NOT be able to write.");

    bdestroy(t1);

    return NULL;
}

int test_task_with_sample(const char *sample_file)
{
    check(SRV, "Server isn't configured.");

    Connection *conn = Connection_create(SRV, 12, 1400, "127.0.0.1");
    check(conn, "Failed to create the conn.");

    conn->fd = open(sample_file, O_RDONLY);
    check(!conn->fd >= 0, "Failed to open the sample file: %s.", sample_file);

    Connection_task(conn);

    return 1;
error:
    return 0;
}

char *test_Connection_task() 
{
    debug(">>>>> XML <<<<<");
    mu_assert(test_task_with_sample("tests/sample.xml"), "xml failed.");
    debug(">>>>> JSON <<<<<");
    mu_assert(test_task_with_sample("tests/sample.json"), "json failed.");
    debug(">>>>> GARBAGE <<<<<");
    mu_assert(test_task_with_sample("tests/sample.garbage"), "garbage failed");
    debug(">>>>>> HTTP <<<<<<");

    mu_assert(test_task_with_sample("tests/sample.http"), "http failed");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    Server_init();
    SRV = Server_create("1999");
    Host *zedshaw_com = Host_create("zedshaw.com");

    Host_add_backend(zedshaw_com, "@chat", strlen("@chat"), BACKEND_HANDLER, NULL);

    Dir *tests = Dir_create("tests/", "/tests/", "index.html");

    Host_add_backend(zedshaw_com, "/tests", strlen("/tests"), BACKEND_DIR, tests);

    Server_set_default_host(SRV, zedshaw_com);

    mu_run_test(test_Connection_create_destroy);
    mu_run_test(test_Connection_deliver);
    mu_run_test(test_Connection_task);

    Server_destroy(SRV);
    // TODO: the above will eventually do this
    Host_destroy(zedshaw_com);
    zmq_term(ZMQ_CTX);
    return NULL;
}

RUN_TESTS(all_tests);

