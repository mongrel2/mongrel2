#include "xrequest.h"
#include "adt/hash.h"
#include "mem/halloc.h"
#include "connection.h"
#include <dlfcn.h>

#define MAX_DISPATCHER_COUNT 128

static hash_t *DISPATCHER_DICT = NULL;

int Xrequest_init()
{
    DISPATCHER_DICT=hash_create(MAX_DISPATCHER_COUNT, (hash_comp_t)bstrcmp, bstr_hash_fun);
    check_mem(DISPATCHER_DICT);

    return 0;
error:
    return -1;
}


int Xrequest_load(Server *srv, bstring load_path, tns_value_t *config)
{
    size_t i;

    if(DISPATCHER_DICT == NULL) {
        check(Xrequest_init() == 0, "Failed to initialize XREQ storage.");
    }

    void *lib=dlopen(bdata(load_path), RTLD_LAZY | RTLD_LOCAL);
    check(lib != NULL, "Failed to load XREQ %s: %s", bdata(load_path), dlerror());

    xrequest_init_cb init = dlsym(lib, "xrequest_init");
    check(init != NULL, "XREQ %s doesn't have an init function.", bdata(load_path));

    size_t nkeys = 0;
    bstring *keys=NULL;

    deliver_function dispatcher = init(srv,load_path, config, &keys, &nkeys);
    check(nkeys >0 && keys != NULL && dispatcher != NULL,
         "Invalid result from XREQ %s init", bdata(load_path));

    for(i=0;i<nkeys;++i) {
        hash_alloc_insert(DISPATCHER_DICT, keys[i], dispatcher);
    }

    return 0;
error:
    return -1;
}

int dispatch_extended_request(Connection *conn,
        bstring key, tns_value_t *value)
{
    check(DISPATCHER_DICT != NULL, "No XREQ dispatchers installed.");
    hnode_t *match = hash_lookup(DISPATCHER_DICT, key);

    check(match != NULL, "Couldn't find XREQ for \"%s\".", bdata(key));

    check(match->hash_data != NULL, "NULL XREQ entry for \"%s\".",bdata(key));
    deliver_function cb = match->hash_data;
    return Connection_deliver_enqueue(conn,cb,value);

error:
    return -1;
}
