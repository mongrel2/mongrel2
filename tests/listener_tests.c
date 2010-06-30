#include "minunit.h"
#include <listener.h>
#include <server.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <task/task.h>
#include <assert.h>


FILE *LOG_FILE = NULL;
Server *SRV = NULL;
const char *FLASH_POLICY = "<policy-file-request/>";
const char *JSON_PING = "@chat {\"type\":\"ping\"}";

char *test_Listener_init() 
{
    mqinit(1);

    Listener_init();

    // this is done to make sure it is set for later tests
    Listener_set_iofuncs(fdread, fdwrite);


    return NULL;
}

char *test_Listener_create_destroy()
{
    Listener *listener = Listener_create(SRV, 12, 1400, "127.0.0.1"); 
    mu_assert(listener != NULL, "Failed to make listener.");

    Listener_destroy(listener);

    return NULL;
}


char *test_Listener_deliver()
{
    bstring t1;

    int rc = Listener_deliver(1, t1 = bfromcstr("TEST"));
    // depending on the platform this will fail or not if send is allowed on files
    mu_assert(rc == 0, "Should be able to write.");

    bdestroy(t1);

    return NULL;
}

int test_task_with_sample(const char *sample_file)
{
    check(SRV, "Server isn't configured.");

    Listener *listener = Listener_create(SRV, 12, 1400, "127.0.0.1");
    check(listener, "Failed to create the listener.");

    listener->fd = open(sample_file, O_RDONLY);
    check(!listener->fd >= 0, "Failed to open the sample file: %s.", sample_file);

    Listener_task(listener);

    return 1;
error:
    return 0;
}

char *test_Listener_task()
{
    assert(SRV->default_host != NULL);

    mu_assert(test_task_with_sample("tests/sample.json"), "json failed.");
    mu_assert(test_task_with_sample("tests/sample.xml"), "xml failed.");
    mu_assert(test_task_with_sample("tests/sample.garbage"), "garbage failed");
    mu_assert(test_task_with_sample("tests/sample.http"), "http failed");

    return NULL;
}

char *test_Listener_parse()
{
    Listener *listener = Listener_create(SRV, 12, 1400, "127.0.0.1");
    strcpy(listener->buf, JSON_PING);
    listener->nread = strlen(listener->buf) + 1;
    listener->fd = 1;

    int rc = Listener_parse(listener);
    mu_assert(rc == 0, "Failed to parse the json message.");
    mu_assert(listener->parser->json_sent, "Didn't actually find the ping.");
    http_parser_init(listener->parser);

    strcpy(listener->buf, FLASH_POLICY);
    listener->nread = strlen(listener->buf) + 1;

    rc = Listener_parse(listener);
    mu_assert(rc == 0, "Failed to parse flash message.");
    mu_assert(listener->parser->socket_started, "Didn't start flash socket.");

    Listener_destroy(listener);

    return NULL;
}


char * all_tests() 
{
    mu_suite_start();

    Server_init();
    SRV = Server_create("1999");
    Host *zedshaw_com = Host_create("zedshaw.com");
    Server_set_default_host(SRV, zedshaw_com);

    mu_run_test(test_Listener_init);
    mu_run_test(test_Listener_create_destroy);
    mu_run_test(test_Listener_deliver);
    mu_run_test(test_Listener_parse);
    mu_run_test(test_Listener_task);

    Server_destroy(SRV);

    // TODO: the above will eventually do this
    Host_destroy(zedshaw_com);
    zmq_term(ZMQ_CTX);

    return NULL;
}

RUN_TESTS(all_tests);

