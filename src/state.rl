#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>


%%{
    machine State;

    access state->;

### actions
    action begin { debug("BEGIN "); }
    action open { debug("OPEN "); }
    action error { debug("ERROR! "); }
    action finish { debug("FINISH "); }
    action close { debug("CLOSE "); }
    action timeout { debug("TIMEOUT"); }

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
    action proxy_finish { debug("PROXY FINISH "); }

    import "events.h";


Proxy := (
        start: ( 
           CONNECT @proxy_connected -> Sending |
           FAILED @proxy_failed -> final |
           REMOTE_CLOSE @proxy_exit_idle |
           TIMEOUT @timeout -> final
        ),

        Connected: (
           REMOTE_CLOSE @proxy_exit_idle |
           REQ_RECV -> Routing |
           TIMEOUT @timeout -> final
        ),

        Routing: (
           HTTP_REQ PROXY @proxy_request -> Sending |
           HTTP_REQ (HANDLER|DIRECTORY) @proxy_exit_routing
        ),

        Sending: (
            REQ_SENT @proxy_req_sent -> Expecting |
            TIMEOUT @timeout -> final
        ),

        Expecting: (
            RESP_RECV @proxy_resp_recv -> Responding |
            TIMEOUT @timeout -> final
        ),

        Responding: ( 
            RESP_SENT @proxy_resp_sent -> Connected |
            TIMEOUT @timeout -> final
        )

     )  >begin %eof(proxy_finish) <err(proxy_error);


Connection = (
        start: ( OPEN @open -> Accepting ),

        Accepting: ( ACCEPT -> Idle ),

        Idle: (
            REQ_RECV HTTP_REQ -> HTTPRouting |
            REQ_RECV MSG_REQ -> MSGRouting |
            REQ_RECV SOCKET_REQ @close -> final |
            MSG_RESP -> Responding |
            CLOSE @close -> final |
            TIMEOUT @timeout -> final 
        ),

        MSGRouting: (
            HANDLER -> Queueing |
            PROXY -> Delivering |
            DIRECTORY -> Responding 
        ),

        HTTPRouting: (
            HANDLER -> Queueing |
            PROXY @{ fgoto Proxy; } |
            DIRECTORY -> Responding
        ),

        Queueing: ( REQ_SENT -> Idle ),

        Delivering: (
            TIMEOUT @timeout -> final |
            REQ_SENT -> Waiting 
        ),

        Waiting: ( 
            TIMEOUT @timeout -> final |
            RESP_RECV -> Responding
        ),

        Responding: (
            TIMEOUT @timeout -> final |
            RESP_SENT -> Idle
        )

        ) >begin %eof(finish) <err(error);

main := (Connection)*;
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


