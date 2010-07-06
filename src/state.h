#ifndef _state_h
#define _state_h

struct State;

typedef int (*state_action_cb)(struct State *state, int event, void *data);

typedef struct StateActions {
    state_action_cb open;
    state_action_cb error;
    state_action_cb finish;
    state_action_cb close;
    state_action_cb parse;
    state_action_cb identify_request;
    state_action_cb route_request;
    state_action_cb send_socket_response;
    state_action_cb msg_to_handler;
    state_action_cb http_to_handler;
    state_action_cb http_to_proxy;
    state_action_cb http_to_directory;
    state_action_cb proxy_deliver;
    state_action_cb proxy_failed;
    state_action_cb proxy_parse;
    state_action_cb proxy_close;
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
