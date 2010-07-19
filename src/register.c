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

#include <register.h>
#include <dbg.h>
#include <adt/hash.h>
#include <task/task.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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
        check_mem(registrations);
    }

    return;

error:
    taskexitall(1);
}

void Register_connect(int fd, int conn_type)
{
    assert(registrations && "Call Register_init.");

    debug("Registering %d ident.", fd);

    hnode_t *hn = hash_lookup(registrations, (void *)(intptr_t)fd);

    if(hn) hash_delete_free(registrations, hn);

    check(hash_alloc_insert(registrations, (void *)(intptr_t)fd, 
                (void *)(intptr_t)conn_type), "Cannot register fd, out of space.");

    debug("Currently registered idents: %d", (int)hash_count(registrations));

    return;

error:
    taskexitall(1);
}


void Register_disconnect(int fd)
{
    assert(registrations && "Call Register_init.");

    hnode_t *hn = hash_lookup(registrations, (void *)(intptr_t)fd);

    if(hn) {
        debug("Unregistering %d", fd);

        hash_delete_free(registrations, hn);

        fdclose(fd);
    } else {
        log_err("Ident %d was unregistered but doesn't exist in registrations.", fd);
    }
}

int Register_ping(int fd)
{
    assert(registrations && "Call Register_init.");

    debug("Ping received for ident %d", fd);
    hnode_t *hn = hash_lookup(registrations, (void *)(intptr_t)fd);

    check_debug(hn, "Ping received but %d isn't actually registerd.", fd);
    // TODO: implement ping and timeout tracking killing dead connects
    
    return 1;

error:
    return 0; 
}


int Register_exists(int fd)
{
    assert(registrations && "Call Register_init.");
    hnode_t *node = hash_lookup(registrations, (void *)(intptr_t)fd);

    if(node) {
        return (int)(intptr_t)hnode_get(node);
    } else {
        return 0;
    }
}

