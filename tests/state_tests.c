#include "minunit.h"
#include <state.h>
#include <events.h>
#include <stdint.h>

FILE *LOG_FILE = NULL;

int test_action_cb(int event, void *data)
{
    int i = (int)(intptr_t)data;

    debug("EVENT[%d]: %s:%d", i, State_event_name(event), event);
    return 1;
}

StateActions test_actions = {
    .open = test_action_cb,
    .error = test_action_cb,
    .finish = test_action_cb,
    .close = test_action_cb,
    .parse = test_action_cb,
    .identify_request = test_action_cb,
    .send_socket_response = test_action_cb,
    .route_request = test_action_cb,
    .msg_to_handler = test_action_cb,
    .http_to_handler = test_action_cb,
    .http_to_proxy = test_action_cb,
    .http_to_directory = test_action_cb,
    .proxy_deliver = test_action_cb,
    .proxy_failed = test_action_cb,
    .proxy_req_parse = test_action_cb,
    .proxy_reply_parse = test_action_cb,
    .proxy_close = test_action_cb
};

/**
 * Runs a bunch of events on the given state, printing out the
 * results, and then returning whether it exited cleanly or not.
 */
int run_events(State *state, const char *name, int *events)
{
    int i = 0;
    int rc = 0;
    State_init(state, &test_actions);

    debug(">>> RUNNING %s", name);

    for(i = 0; events[i] != 0; i++) {
        rc = State_exec(state, events[i], (void *)(intptr_t)i);
        check(State_finish(state) != -1, "Failed on processing %d event.", events[i]);
    }

    debug("<<< FINAL RESULT: %d, finished: %d", rc, State_finish(state));
    return State_finish(state);

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

char *test_State_msg()
{
    State state;

    // Simulates the most basic MSG message request for a handler.
    RUN(msg_handler, 
            OPEN, ACCEPT, 
            REQ_RECV, MSG_REQ, HANDLER, REQ_SENT, CLOSE);

    // Simulates a basic socket request start
    RUN(socket_start, OPEN, ACCEPT, REQ_RECV, SOCKET_REQ, RESP_SENT, CLOSE);

    // simulates two requests then a close
    RUN(msg_handler_2_req, 
            OPEN, ACCEPT, 
            REQ_RECV, MSG_REQ, HANDLER, REQ_SENT,
            REQ_RECV, MSG_REQ, HANDLER, REQ_SENT,
            CLOSE);


    return NULL;
}


char *test_State_http() 
{
    State state;

    // Simulates doing a basic HTTP request then closing the connection.
    RUN(http_dir,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, DIRECTORY, RESP_SENT, CLOSE);

    // Simulates two keep-alive handler requests then a close.
    RUN(http_handler,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, HANDLER, REQ_SENT,
            REQ_RECV, HTTP_REQ, HANDLER, REQ_SENT, CLOSE);

    // Simulates two requests over a proxy connection followed by
    // the remote closing the connection so we have to shutdown.
    RUN(http_proxy,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, PROXY, CONNECT, 
            REQ_SENT, REQ_RECV, HTTP_REQ, REQ_SENT, REQ_RECV, REMOTE_CLOSE,
            CLOSE);

    // Simulates a proxy connect that needs to exit after a 
    // handler request was issued and there's a bit of data left.
    RUN(http_proxy_handler,
            OPEN, ACCEPT,
            REQ_RECV, HTTP_REQ, PROXY, CONNECT, 
            REQ_SENT, REQ_RECV,
            HANDLER, REQ_SENT, CLOSE);

    return NULL;
}


char *test_State_exec_error() 
{
    State state;

    // Basic client opens, then closes right away
    FAILS(failed_connect, OPEN, CLOSE);

    // Proxy error where client closed connection mid-connect
    FAILS(failed_proxy, 
            OPEN, ACCEPT, 
            REQ_RECV, HTTP_REQ, PROXY, CONNECT, CLOSE);

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_State_exec_error);
    mu_run_test(test_State_msg);
    mu_run_test(test_State_http);

    return NULL;
}

RUN_TESTS(all_tests);

