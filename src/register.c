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
#include <task/task.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


static Registration REGISTRATIONS[MAX_REGISTERED_FDS];
// this has to stay uint16_t so we wrap around
static uint16_t REG_COUNT = 0;
static uint16_t REG_ID_TO_FD[MAX_REGISTERED_FDS];

void Register_init()
{
    memset(REGISTRATIONS, 0, sizeof(REGISTRATIONS));
    memset(REG_ID_TO_FD, 0, sizeof(REG_ID_TO_FD));
}

static inline void Register_clear(Registration *reg)
{
    reg->conn_type = 0;
    reg->last_ping = 0;
    REG_ID_TO_FD[reg->id] = -1;
}

int Register_connect(int fd, int conn_type)
{
    int rc = 0;
    assert(fd < MAX_REGISTERED_FDS && "FD given to register is greater than max.");
    assert(conn_type != 0 && "conn_type can't be 0");

    Registration *reg = &REGISTRATIONS[fd];

    if(reg->conn_type) {
        debug("Looks like stale registration in %d, kill it before it gets out.", fd);
        // a new Register_connect came in, but we haven't disconnected the previous
        Register_clear(reg);
        check(rc != -1, "Weird error, tried to disconnect something that exists then got an error: %d", fd);
    }

    reg->conn_type = conn_type;
    reg->last_ping = time(NULL);
    
    // purposefully want overflow on these
    reg->id = REG_COUNT++;
    REG_ID_TO_FD[reg->id] = fd;

    return reg->id;
error:
    return -1;
}


int Register_disconnect(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for disconnect: %d", fd);

    Registration *reg = &REGISTRATIONS[fd];
    check(reg->conn_type != 0, "Attempt to unregister FD %d which is already gone.", fd);

    Register_clear(reg);
    fdclose(fd);

    return reg->id;

error:
    fdclose(fd);
    return -1;
}

int Register_ping(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for disconnect: %d", fd);
    Registration *reg = &REGISTRATIONS[fd];

    check(reg->conn_type != 0, "Attemp to ping an FD that isn't registered: %d", fd);

    reg->last_ping = time(NULL);
    return reg->last_ping;

error:
    return -1;
}


int Register_fd_exists(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for disconnect: %d", fd);

    return REGISTRATIONS[fd].conn_type;
error:
    return 0;
}


int Register_fd_for_id(int id)
{
    assert(id < MAX_REGISTERED_FDS && "Ident (id) given to register is greater than max.");
    return REG_ID_TO_FD[id];
}

int Register_id_for_fd(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "FD given to register is greater than max.");
    return REGISTRATIONS[fd].id;
}


bstring Register_info()
{
    int i = 0;
    int total = 0;
    bstring result = bfromcstr("{");
    time_t now = time(NULL);

    for(i = 0; i < MAX_REGISTERED_FDS; i++) {
        if(REGISTRATIONS[i].conn_type) {
            bformata(result, "\"%d\":%d,", REGISTRATIONS[i].id,
                    now - REGISTRATIONS[i].last_ping);
            total++;
        }
    }

    bformata(result, "\"total\": %d}", total);
    return result;
}
