#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>


%%{
    machine StateActions;

    access state->;

### actions
    action begin { debug("BEGIN "); }
    action open { debug("OPEN "); }
    action error { debug("ERROR! "); }
    action finish { debug("FINISH "); }
    action close { debug("CLOSE "); }
    action timeout { debug("TIMEOUT"); }
    action accepted { debug("ACCEPTED"); }
    action http_req { debug("HTTP REQUEST"); }
    action msg_req { debug("MSG REQUEST"); }
    action msg_resp { debug("MSG RESP"); }
    action msg_to_handler { debug("MSG TO HANDLER"); }
    action msg_to_proxy { debug("MSG TO PROXY"); }
    action msg_to_directory { debug("MSG TO DIRECTORY"); }

    action http_to_handler { debug("HTTP TO HANDLER"); }
    action http_to_proxy { debug("HTTP TO PROXY"); fgoto Proxy; }
    action http_to_directory { debug("HTTP TO DIRECTORY"); }
    action req_sent { debug("REQ SENT"); }
    action resp_sent { debug("RESP SENT"); }
    action resp_recv { debug("RESP RECV"); }

### proxy actions
    action proxy_connected { debug("PROXY CONNECTED"); }
    action proxy_failed { debug("PROXY FAILED"); }
    action proxy_request { debug("PROXY REQUEST"); }
    action proxy_req_sent { debug("PROXY REQUEST SENT"); }
    action proxy_resp_sent { debug("PROXY RESP SENT"); }
    action proxy_resp_recv { debug("PROXY RESP RECV"); }


### exit modes for proxy
    action proxy_exit_idle { debug("PROXY EXIT IDLE"); fgoto Connection::Idle; }
    action proxy_exit_routing { debug("PROXY EXIT ROUTING"); fhold; fgoto Connection::HTTPRouting; }

    action proxy_error { debug("PROXY ERROR! "); }

    include State "state_machine.rl";
}%%

%% write data;

int State_init(State *state)
{
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

int State_exec(State *state, int event)
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
    assert(event > EVENT_START && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - EVENT_START - 1];
}
