#ifndef _filter_h
#define _filter_h

#include "connection.h"
#include "bstring.h"
#include "events.h"
#include "tnetstrings.h"

extern int FILTERS_ACTIVATED;

typedef StateEvent (*filter_cb)(int next, Connection *conn, tns_value_t *config);
typedef StateEvent* (*filter_init_cb)(Server *srv, bstring load_path, int *out_nstates);

typedef struct Filter {
    int state;
    filter_cb cb;
    bstring load_path;
    tns_value_t *config;
} Filter;

int Filter_init();
void Filter_destroy();
int Filter_run(StateEvent next, Connection *conn);
int Filter_add(StateEvent state, filter_cb cb, bstring load_path, tns_value_t *config);
int Filter_load(Server *srv, bstring load_path, tns_value_t *config);

StateEvent *Filter_state_list(StateEvent *states, int length);

#define Filter_states_length(L) (sizeof(L) / sizeof(StateEvent))

#define Filter_activated() (FILTERS_ACTIVATED)

#endif
