#include <filter.h>
#include <dbg.h>

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{
    return CLOSE;
}


StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates, tns_value_t *config)
{
    StateEvent states[] = {HANDLER};
    *out_nstates = Filter_states_length(states);
    check(*out_nstates == 1, "Wrong state array length.");

    return Filter_state_list(states, *out_nstates);

error:
    return NULL;
}

