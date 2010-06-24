#include "minunit.h"
#include <listener.h>
#include <server.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <task/task.h>


FILE *LOG_FILE = NULL;
Server *SRV = NULL;
const char *FLASH_POLICY = "<policy-file-request/>";
const char *JSON_PING = "{\"type\":\"ping\"}";

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
    int rc = Listener_deliver(1, "TEST", strlen("TEST"));
    // depending on the platform this will fail or not if send is allowed on files
    mu_assert(rc == 0, "Should be able to write.");

    return NULL;
}

char *test_Listener_task()
{
    Listener *listener = Listener_create(SRV, 12, 1400, "127.0.0.1");
    listener->fd = open("tests/sample.json", O_RDONLY);
    mu_assert(listener->fd >= 0, "Failed to open the tests/sample json file.");

    Listener_task(listener);
    // it's destroyed now
    
    mu_assert(!Register_exists(12), "Didn't unregister the socket.");

    listener = Listener_create(SRV, 12, 1400, "127.0.0.1");
    listener->fd = open("tests/sample.xml", O_RDONLY);
    mu_assert(listener->fd >= 0, "Failed to open tests/sample.xml file.");

    Listener_task(listener);

    mu_assert(!Register_exists(12), "Didn't unregister the socket.");

    // TODO: add an http request to the mix

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

    Server_destroy(SRV);
    Listener_destroy(listener);

    return NULL;
}


char * all_tests() 
{
    mu_suite_start();

    Server_init();
    SRV = Server_create("19999");

    mu_run_test(test_Listener_init);
    mu_run_test(test_Listener_create_destroy);
    mu_run_test(test_Listener_deliver);
    mu_run_test(test_Listener_parse);
    mu_run_test(test_Listener_task);

    return NULL;
}

RUN_TESTS(all_tests);

