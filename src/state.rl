#include <stdlib.h>
#include <stdio.h>
#include <state.h>
#include <dbg.h>


%%{
  machine State;
  access state->;

### actions
  action begin { debug("BEGIN "); }
  action open { debug("OPEN "); }
  action connected { debug("CONNECTED"); }
  action error { debug("ERROR! "); }
  action finish { debug("FINISH "); }
  action close { debug("CLOSE "); }
  action recv { debug("RECV "); }
  action sent { debug("SENT "); }
  action service { debug("SERVICE"); }
  action delivered { debug("DELIVERED "); }

### events

  import "events.h";

### state chart
   Connection = (
    start: ( OPEN @open -> Accepting ),

    Accepting: ( ACCEPT @connected -> Idle ),

    Idle: (
      REQ_RECV @recv -> Delivering |
      RESP_RECV @service -> Responding |
      CLOSE @close -> final
    ),

    Delivering: ( REQ_SENT @delivered -> Idle ),

    Responding: ( RESP_SENT @sent -> Idle )

  ) >begin %finish <err(error);

  main := (Connection %{ debug("\n"); } )*;
}%%

%% write data;

int State_init(State *state)
{
  %% write init;
  return 1;
}

inline int State_invariant(State *state, int event)
{
  if ( state->cs == State_error ) {
    return -1;
  }

  if ( state->cs >= State_first_final )
    return 1;
  
  return 0;
}

int State_exec(State *state, int event)
{
  const char *p = (const char *)&event;
  const char *pe = p+1;
  const char *eof = NULL;

  %% write exec;

  return State_invariant(state, event);
}

int State_finish(State *state)
{
  return State_invariant(state, 0);
}


