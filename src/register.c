#include <register.h>
#include <dbg.h>
#include <adt/hash.h>
#include <task/task.h>
#include <assert.h>


hash_t *registrations = NULL;

static hash_val_t fd_hash_func(const void *fd)
{
    return (hash_val_t)fd;
}

static int fd_comp_func(const void *a, const void *b)
{
    return a - b;
}


void Register_init()
{
    if(!registrations) {
        registrations = hash_create(HASHCOUNT_T_MAX, fd_comp_func, fd_hash_func);
        check(registrations, "Failed creating registrations store.");
    }

error:
    taskexitall(1);
}

void Register_connect(int fd)
{
    assert(registrations && "Call Register_init.");

    debug("Registering %d ident.", fd);

    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) hash_delete_free(registrations, hn);

    check(hash_alloc_insert(registrations, (void *)fd, NULL), "Cannot register fd, out of space.");

    debug("Currently registered idents: %d", hash_count(registrations));

    return;

error:
    taskexitall(1);
}


void Register_disconnect(int fd)
{
    assert(registrations && "Call Register_init.");

    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    if(hn) {
        debug("Unregistering %d", fd);

        hash_delete_free(registrations, hn);

        if(close(fd) == -1) {
            log_err("Failed on close for ident %d, not sure why.", fd);
        }
    } else {
        log_err("Ident %d was unregistered but doesn't exist in registrations.", fd);
    }
}

int Register_ping(int fd)
{
    assert(registrations && "Call Register_init.");

    debug("Ping received for ident %d", fd);
    hnode_t *hn = hash_lookup(registrations, (void *)fd);

    check(hn, "Ping received but %d isn't actually registerd.", fd);
    // TODO: increment its ping time
    
    return 1;

error:
    return 0; 
}


int Register_exists(int fd)
{
    hash_lookup(registrations, (void *)fd);
    assert(registrations && "Call Register_init.");
}

