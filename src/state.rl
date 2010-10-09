/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>
#include <events.h>
#include <assert.h>

#define CALL(A, C) if(state->actions && state->actions->A) next = state->actions->A(C, data)

%%{
    machine StateActions;
    alphtype int;

    access state->;

### actions
    action open { CALL(open, fc); }
    action error { CALL(error, fc); }
    action close { CALL(close, fc); }
    action parse { CALL(parse, fc); }
    action register_request { CALL(register_request, fc); }
    action identify_request { CALL(identify_request, fc); }
    action send_socket_response { CALL(send_socket_response, fc); }
    action route_request { CALL(route_request, fc); }
    action msg_to_handler { CALL(msg_to_handler, fc); }

    action http_to_handler { CALL(http_to_handler, fc); }
    action http_to_proxy { CALL(http_to_proxy, fc); fgoto Proxy; }
    action http_to_directory { CALL(http_to_directory, fc); }

### proxy actions
    action proxy_deliver { CALL(proxy_deliver, fc); }
    action proxy_failed { CALL(proxy_failed, fc); }
    action proxy_send_request { CALL(proxy_send_request, fc); }
    action proxy_send_response { CALL(proxy_send_response, fc); }
    action proxy_reply_parse { CALL(proxy_reply_parse, fc); }
    action proxy_req_parse { CALL(proxy_req_parse, fc); }
    action proxy_close { CALL(proxy_close, fc); }


### exit modes for proxy
    action proxy_exit_idle {
        fhold;
        fgoto Connection::Idle; 
    }
    action proxy_exit_routing {
        CALL(proxy_close, fc);
        fhold;
        fgoto Connection::HTTPRouting; 
    }

    include State "state_machine.rl";
}%%

%% write data;

int State_init(State *state, StateActions *actions)
{
    state->actions = actions;

    %% write init;
    return 1;
}

int State_invariant(State *state)
{
    if ( state->cs == %%{ write error; }%% ) {
        return -1;
    }

    if ( state->cs >= %%{ write first_final; }%% ) {
        return 1;
    }

    return 0;
}

int State_exec(State *state, int event, Connection *conn)
{
    int event_queue[2] = {0};
    event_queue[0] = event;
    int next = 0;

    const int *p = event_queue;
    const int *pe = p+1;
    const int *eof = event == CLOSE ? pe : NULL;

    %% write exec;

    return next;
}


/* Do not access these directly or alter their order EVER.  */
const char *EVENT_NAMES[] = {
    "FINISHED",
    "ACCEPT",
    "CLOSE",
    "CONNECT",
    "DIRECTORY",
    "FAILED",
    "HANDLER",
    "HTTP_REQ",
    "MSG_REQ",
    "OPEN",
    "PROXY",
    "REMOTE_CLOSE",
    "REQ_RECV",
    "REQ_SENT",
    "RESP_SENT",
    "SOCKET_REQ",
    "TIMEOUT"};

const char *State_event_name(int event)
{
    if(event == 0) event = FINISHED;

    assert(event >= FINISHED && event < EVENT_END && "Event is outside range.");

    return EVENT_NAMES[event - FINISHED];
}
