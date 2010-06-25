#include <host.h>
#include <string.h>
#include <dbg.h>
#include <assert.h>

Host *Host_create(const char *name)
{
    size_t len = strlen(name);

    check(len < MAX_HOST_NAME, "Host name too long (max %d): '%s'\n", 
            MAX_HOST_NAME, name);

    Host *host = calloc(sizeof(Host), 1);
    check(host, "Out of memory error.");

    strncpy(host->name, name, MAX_HOST_NAME);
    host->name[len] = '\0';

    host->routes = RouteMap_create();
    check(host->routes, "Failed to create host route map for %s.", name);
    
    return host;

error:
    return NULL;
}


void Host_destroy(Host *host)
{
    free(host);
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
    } else {
        sentinel("Invalid proxy type given: %d", type);
    }

    debug("Putting backend into %s with pointer: %p", host->name, backend);

    int rc = RouteMap_insert(host->routes, path, path_len, backend);
    check(rc == 0, "Failed to insert into host %s route map.", host->name);

    return 0;
    
error:
    return -1;
}


Backend *Host_match(Host *host, const char *target, size_t len)
{
    // TODO: figure out the best policy, longest? first? all?
    Route *found = NULL;

    list_t *results = RouteMap_match(host->routes, target, len);

    lnode_t *n = list_first(results);
    if(n) {
        found = lnode_get(n);
        assert(found && "RouteMap returned a list node with NULL.");
        debug("Found backend at %.*s", found->length, found->pattern);
    }

    list_destroy_nodes(results);
    list_destroy(results);

    if(found) {
        assert(found->data && "Invalid value for stored route.");
        return found->data;
    } else {
        return NULL;
    }
}

