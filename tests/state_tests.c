#include "minunit.h"
#include <state.h>
#include <events.h>

FILE *LOG_FILE = NULL;


/**
 * Runs a bunch of events on the given state, printing out the
 * results, and then returning whether it exited cleanly or not.
 */
int run_events(State *state, const char *name, int *events)
{
    int i = 0;
    int rc = 0;
    State_init(state);

    debug(">>> RUNNING %s", name);

    for(i = 0; events[i] != 0; i++) {
        debug("EVENT[%d]: %d", i, events[i]);
        rc = State_exec(state, events[i]);
        check(rc != -1, "Failed on processing %d event.", events[i]);
    }

    debug("<<< FINAL RESULT: %d", rc);
    return rc == 1;
error:
    return 0;
}

/**
 * A fancy macro that just sets up a test run with an array of 
 * integers matching the events.h events enum.
 */
#define RUN(N, ...) int N[] = { __VA_ARGS__, 0 };\
          mu_assert(run_events(&state, "TEST " #N, N), "Test " #N " failed.");

/**
 * Same thing but should fail.
 */
#define FAILS(N, ...) int N[] = { __VA_ARGS__, 0 };\
          mu_assert(!run_events(&state, "TEST " #N, N), "Test " #N " failed.");

char *test_State_json_workflow()
{
    State state;

    /**
     * Simulates the most basic JSON message request for a handler.
     */
    RUN(json_handler, 
            OPEN, ACCEPT, 
            REQ_RECV, JSON_REQ, HANDLER, REQ_SENT, RESP_RECV, RESP_SENT,
            CLOSE);

    return NULL;
}


char *test_State_http_workflow() 
{
    State state;

    /**
     * Simulates doing a basic HTTP request then closing the connection.
     */
    RUN(http_dir,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, DIRECTORY, RESP_SENT, CLOSE);

    /**
     * Simulates two requests over a proxy connection followed by
     * the remote closing the connection so we have to shutdown.
     */
    RUN(http_proxy,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, PROXY, CONNECT, REQ_SENT, RESP_RECV, RESP_SENT,
            HTTP_REQ, PROXY, REQ_SENT, RESP_RECV, RESP_SENT, REMOTE_CLOSE,
            CLOSE);

    /**
     * Simulates a proxy connection that is then routed to a directory
     * access, so has to change from proxying to serving a file.
     */
    RUN(http_proxy_dir, 
            OPEN, ACCEPT, REQ_RECV, 
            HTTP_REQ, PROXY, CONNECT, REQ_SENT, RESP_RECV, RESP_SENT, 
            HTTP_REQ, DIRECTORY, RESP_SENT, CLOSE);

    return NULL;
}


char *test_State_exec_error() 
{
    State state;

    FAILS(failed_connect, OPEN, CLOSE);

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_State_exec_error);
    mu_run_test(test_State_json_workflow);
    mu_run_test(test_State_http_workflow);

    return NULL;
}

RUN_TESTS(all_tests);

