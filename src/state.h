#ifndef _state_h
#define _state_h

struct State;

typedef int (*state_action_cb)(struct State *state, int event, void *data);

typedef struct StateActions {
    state_action_cb begin;
    state_action_cb open;
    state_action_cb error;
    state_action_cb finish;
    state_action_cb close;
    state_action_cb timeout;
    state_action_cb accepted;
    state_action_cb http_req;
    state_action_cb msg_req;
    state_action_cb msg_resp;
    state_action_cb msg_to_handler;
    state_action_cb msg_to_proxy;
    state_action_cb msg_to_directory;
    state_action_cb http_to_handler;
    state_action_cb http_to_proxy;
    state_action_cb http_to_directory;
    state_action_cb req_sent;
    state_action_cb resp_sent;
    state_action_cb resp_recv;
    state_action_cb proxy_connected;
    state_action_cb proxy_failed;
    state_action_cb proxy_request;
    state_action_cb proxy_req_sent;
    state_action_cb proxy_resp_sent;
    state_action_cb proxy_resp_recv;
    state_action_cb proxy_exit_idle;
    state_action_cb proxy_exit_routing;
    state_action_cb proxy_error;
} StateActions;

typedef struct State {
    int cs;
    void *data;
    StateActions *actions;
} State;

int State_exec(State *state, int event, void *data);
int State_finish(State *state);
int State_init(State *state, StateActions *actions);

const char *State_event_name(int event);

#endif
