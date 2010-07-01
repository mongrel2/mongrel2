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
    @proxy_connected { debug("PROXY CONNECTED"); }
    @proxy_failed { debug("PROXY FAILED"); }

### exit modes for proxy
    action exit_idle {fgoto Connection::Idle; }
    action exit_routing { fhold; fgoto Connection::HTTPRouting; }

    import "events.h";


Proxy := (
        start: ( 
           CONNECT @proxy_connected -> Sending |
           FAILED @proxy_failed -> final |
           REMOTE_CLOSE @exit_idle |
           TIMEOUT @timeout -> final
        ),

        Connected: (
           REMOTE_CLOSE @exit_idle |
           REQ_RECV -> Routing |
           TIMEOUT @timeout -> final
        ),

        Routing: (
           HTTP_REQ PROXY -> Sending |
           HTTP_REQ (HANDLER|DIRECTORY) @exit_routing
        ),

        Sending: (
            REQ_SENT -> Expecting |
            TIMEOUT @timeout -> final
        ),

        Expecting: (
            RESP_RECV -> Responding |
            TIMEOUT @timeout -> final
        ),

        Responding: ( 
            RESP_SENT -> Connected |
            TIMEOUT @timeout -> final
        )

     )  >begin %eof(finish) <err(error);


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


