#include <host.h>
#include <string.h>
#include <dbg.h>
#include <routing.h>

Host *Host_create(const char *name)
{
    size_t len = strlen(name);

    check(len < MAX_HOST_NAME, "Host name too long (max %d): '%s'\n", 
            MAX_HOST_NAME, name);

    Host *host = calloc(sizeof(Host), 1);
    check(host, "Out of memory error.");

    strncpy(host->name, name, MAX_HOST_NAME);
    host->name[len] = '\0';
    
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
        check(0, "Invalid proxy type given: %d", type);
    }

    int rc = RouteMap_insert(host->routes, path, path_len, backend);
    check(rc == 0, "Failed to insert into host %s route map.", host->name);

    return 0;
    
error:
    return -1;
}


Backend *Host_match(Host *host, const char *target, size_t len)
{
    // TODO: figure out the best policy, longest? first? all?
    Backend *found = NULL;

    list_t *results = RouteMap_match(host->routes, target, len);
    if(!list_isempty(results)) {
        found = (Backend *)lnode_get(list_first(results));
    }

    list_destroy_nodes(results);
    list_destroy(results);

    return found;
}

