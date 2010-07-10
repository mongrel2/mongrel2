#include <host.h>
#include <string.h>
#include <dbg.h>
#include <assert.h>
#include <mem/halloc.h>

Host *Host_create(const char *name)
{
    Host *host = h_calloc(sizeof(Host), 1);
    check(host, "Out of memory error.");

    host->name = bfromcstr(name);
    check(blength(host->name) < MAX_HOST_NAME, "Host name too long (max %d): '%s'\n", 
            MAX_HOST_NAME, name);

    host->routes = RouteMap_create();
    check(host->routes, "Failed to create host route map for %s.", name);
    
    return host;

error:
    return NULL;
}


void Host_destroy(Host *host)
{
    if(host) {
        bdestroy(host->name);
        RouteMap_destroy(host->routes);
        h_free(host);
    }
}



int Host_add_backend(Host *host, const char *path, size_t path_len, BackendType type, void *target)
{
    Backend *backend = calloc(sizeof(Backend), 1);
    check(backend, "Out of Memory");

    backend->type = type;

    if(type == BACKEND_HANDLER) {
        backend->target.handler = target;
    } else if(type == BACKEND_PROXY) {
        backend->target.proxy = target;
    } else if(type == BACKEND_DIR) {
        backend->target.dir = target;
    } else {
        sentinel("Invalid proxy type given: %d", type);
    }

    int rc = RouteMap_insert(host->routes, blk2bstr(path, path_len), backend);
    check(rc == 0, "Failed to insert into host %s route map.", bdata(host->name));

    return 0;
    
error:
    return -1;
}


Backend *Host_match_backend(Host *host, bstring target)
{
    Route *found = NULL;

    list_t *results = RouteMap_match(host->routes, target);

    if(list_count(results) > 0) {
        found = lnode_get(list_first(results));
    }

    list_destroy_nodes(results);
    list_destroy(results);

    if(found) {
        debug("Found backend at %s", bdata(found->pattern));
        assert(found->data && "Invalid value for stored route.");
        return found->data;
    } else {
        return NULL;
    }
}

