#include <host.h>
#include <string.h>
#include <dbg.h>

Host *Host_create(const char *name)
{
    size_t len = strlen(name);

    check(len < MAX_HOST_NAME, "Host name too long (max %d): '%s'\n", 
            MAX_HOST_NAME, name);

    Host *host = calloc(sizeof(Host), 1);
    check(host, "Out of memory error.");

    strncpy(host->name, name, len);
    host->name[len] = '\0';
    
    return host;

error:
    return NULL;
}


void Host_destroy(Host *host)
{
    // TODO: free the tst too
    free(host);
}


