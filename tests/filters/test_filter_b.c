#include <filter.h>
#include <dbg.h>

StateEvent filter_transition(StateEvent state, Connection *conn)
{
	conn->rport += 8;
    return state;
}


StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates)
{
    StateEvent states[] = {CONNECT};
    *out_nstates = Filter_states_length(states);
    check(*out_nstates == 1, "Wrong state array length.");

    return Filter_state_list(states, *out_nstates);

error:
    return NULL;
}

