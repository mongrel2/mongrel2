#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>

#define CALL(A, C) if(state->actions && state->actions->A) state->actions->A(state, C, data)

%%{
    machine StateActions;

    access state->;

### actions
    action begin { CALL(begin, fc); }
    action open { CALL(open, fc); }
    action error { CALL(error, fc); }
    action finish { CALL(finish, fc); }
    action close { CALL(close, fc); }
    action timeout { CALL(timeout, fc); }
    action accepted { CALL(accepted, fc); }
    action http_req { CALL(http_req, fc); }
    action msg_req { CALL(msg_req, fc); }
    action msg_resp { CALL(msg_resp, fc); }
    action msg_to_handler { CALL(msg_to_handler, fc); }
    action msg_to_proxy { CALL(msg_to_proxy, fc); }
    action msg_to_directory { CALL(msg_to_directory, fc); }

    action http_to_handler { CALL(http_to_handler, fc); }
    action http_to_proxy { CALL(http_to_proxy, fc); fgoto Proxy; }
    action http_to_directory { CALL(http_to_directory, fc); }
    action req_sent { CALL(req_sent, fc); }
    action resp_sent { CALL(resp_sent, fc); }
    action resp_recv { CALL(resp_recv, fc); }

### proxy actions
    action proxy_connected { CALL(proxy_connected, fc); }
    action proxy_failed { CALL(proxy_failed, fc); }
    action proxy_request { CALL(proxy_request, fc); }
    action proxy_req_sent { CALL(proxy_req_sent, fc); }
    action proxy_resp_sent { CALL(proxy_resp_sent, fc); }
    action proxy_resp_recv { CALL(proxy_resp_recv, fc); }


### exit modes for proxy
    action proxy_exit_idle {
        CALL(proxy_exit_idle, fc);
        fgoto Connection::Idle; 
    }
    action proxy_exit_routing {
        CALL(proxy_exit_routing, fc);
        fhold;
        fgoto Connection::HTTPRouting; 
    }
    action proxy_error { CALL(proxy_error, fc); }

    include State "state_machine.rl";
}%%

%% write data;

int State_init(State *state, StateActions *actions)
{
    state->actions = actions;

    %% write init;
    return 1;
}

inline int State_invariant(State *state, int event)
{
    if ( state->cs == %%{ write error; }%% ) {
        return -1;
    }

    if ( state->cs >= %%{ write first_final; }%% ) {
        return 1;
    }

    return 0;
}

int State_exec(State *state, int event, void *data)
{
    const char *p = (const char *)&event;
    const char *pe = p+1;
    const char *eof = event == CLOSE ? pe : NULL;

    %% write exec;

    return State_invariant(state, event);
}

int State_finish(State *state)
{
    return State_invariant(state, 0);
}


/* Do not access these directly or alter their order EVER.  */
const char *EVENT_NAMES[] = {
    "ACCEPT",
    "CLOSE",
    "CONNECT",
    "DIRECTORY",
    "FAILED",
    "HANDLER",
    "HTTP_REQ",
    "MSG_REQ",
    "MSG_RESP",
    "OPEN",
    "PROXY",
    "REMOTE_CLOSE",
    "REQ_RECV",
    "REQ_SENT",
    "RESP_RECV",
    "RESP_SENT",
    "SOCKET_REQ",
    "TIMEOUT"};

const char *State_event_name(int event)
{
    // TODO: find out why the hell the FSM is actually giving us this

    if(event == 0) {
        return "NUL";
    }

    assert(event > EVENT_START && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - EVENT_START - 1];
}
