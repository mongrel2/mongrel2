#include "minunit.h"
#include <state.h>
#include <events.h>

FILE *LOG_FILE = NULL;

char *test_State_exec() 
{
    State state;
    int i = 0;
    int rc = 0;
    int events[] = {OPEN, ACCEPT, REQ_RECV, REQ_SENT, RESP_RECV, 
        RESP_SENT, CLOSE, 0};

    State_init(&state);
    for(i = 0; events[i] != 0; i++) {
        rc = State_exec(&state, events[i]);
        debug("rc = %d after event %d", rc, events[i]);
        mu_assert(rc != -1, "Failure in event processing.");
    }
    mu_assert(rc == 1, "Should NOT be running");

    return NULL;
}


char *test_State_exec_error() 
{
    State state;
    int i = 0;
    int rc = 0;
    int events[] = {OPEN, CLOSE, 0};

    State_init(&state);
    for(i = 0; events[i] != 0; i++) {
        rc = State_exec(&state, events[i]);
        debug("rc = %d after event %d", rc, events[i]);
    }
    mu_assert(rc == -1, "Should have an error.");

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_State_exec);
    mu_run_test(test_State_exec_error);

    return NULL;
}

RUN_TESTS(all_tests);

