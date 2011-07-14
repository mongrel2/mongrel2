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

#include "filter.h"
#include "adt/darray.h"
#include "mem/halloc.h"
#include <dlfcn.h>

// need a darray mapped to darray of filter callbacks

static darray_t *REGISTERED_FILTERS = NULL;
int FILTERS_ACTIVATED = 0;

int Filter_init()
{
    REGISTERED_FILTERS = darray_create(sizeof(darray_t *), EVENT_END - EVENT_MIN);
    check_mem(REGISTERED_FILTERS);

    return 0;
error:
    return -1;
}

/**
 * Mostly used in testing, reloading filters won't be supported.
 */
void Filter_destroy()
{
    darray_destroy(REGISTERED_FILTERS);
    REGISTERED_FILTERS = NULL;
}

static inline darray_t *Filter_lookup(StateEvent next)
{
    return darray_get(REGISTERED_FILTERS, next - EVENT_MIN);
}

static inline darray_t *Filter_lookup_create(StateEvent next)
{
    darray_t *filts = Filter_lookup(next);

    if(filts == NULL) {
        // lazy load them
        filts = darray_create(sizeof(Filter), 10);
        check_mem(filts);
        darray_set(REGISTERED_FILTERS, next - EVENT_MIN, filts);
    }

    return filts;
error:
    return NULL;
}


int Filter_run(StateEvent next, Connection *conn)
{
    int i = 0;
    StateEvent res = next;
    check(REGISTERED_FILTERS != NULL, "No filters loaded yet, don't call this.");

    darray_t *filters = Filter_lookup(next);

    if(filters != NULL) {
        for(i = 0; i < darray_end(filters) && res == next; i++) {
            Filter *filter = darray_get(filters, i);
            check(filter != NULL, "Expected to get a filter record but got NULL.");

            res = filter->cb(next, conn, filter->config);
            check(res >= CLOSE && res < EVENT_END,
                    "Filter %s returned invalid event: %d", bdata(filter->load_path), res);
        }
    }

    return res;

error:
    return -1;
}

int Filter_add(StateEvent state, filter_cb cb, bstring load_path, tns_value_t *config)
{
    darray_t *filters = Filter_lookup_create(state);
    check(filters != NULL, "Invalid filter state: %d given for filter %s",
            state, bdata(load_path));

    Filter *filter = darray_new(filters);
    check_mem(filter);

    filter->state = state;
    filter->cb = cb;
    filter->load_path = bstrcpy(load_path);
    filter->config = config;

    darray_attach(filters, filter);
    darray_push(filters, filter);

    return 0;
error:
    return -1;
}


int Filter_load(Server *srv, bstring load_path, tns_value_t *config)
{
    int i = 0;

    if(REGISTERED_FILTERS == NULL) {
        check(Filter_init() == 0, "Failed to initialize filter storage.");
        FILTERS_ACTIVATED = 1;
    }

    // TODO: probably too aggressive but we'll try this for now
    void *lib = dlopen(bdata(load_path), RTLD_LAZY | RTLD_LOCAL);
    check(lib != NULL, "Failed to load filter %s: %s.", bdata(load_path), dlerror());

    // get the Filter_init function and run it to init the filter (duh)
    filter_init_cb init = dlsym(lib, "filter_init");
    check(init != NULL, "Filter %s doesn't have an init function.", bdata(load_path));

    int nstates = 0;
    StateEvent *states = init(srv, load_path, &nstates);
    check(states != NULL, "Init for %s return NULL failure.", bdata(load_path));
    check(nstates > 0, "Init for %s return <= 0 states, nothing to do.", bdata(load_path));

    filter_cb cb = dlsym(lib, "filter_transition");
    check(cb != NULL, "No Filter_transition defined in %s, fail.", bdata(load_path));

    // now roll through all the states this thing will trigger on and register its callback
    for(i = 0; i < nstates; i++) {
        StateEvent state = states[i];
        check(state >= CLOSE && state < EVENT_END,
                "Invalid state return by %s Filter_init: %d", bdata(load_path), state);

        // now create the internal Filter record for activity later.
        check(Filter_add(state, cb, load_path, config) == 0, "Failed to add filter:state %s:%d",
                bdata(load_path), state);
    }

    return 0;
error:
    return -1;
}


StateEvent *Filter_state_list(StateEvent *states, int length)
{
    StateEvent *new_states = calloc(sizeof(StateEvent), length);
    memcpy(new_states, states, length * sizeof(StateEvent));

    return new_states;
}
