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

#define MAX_REGISTERED_FDS  64 * 1024

static Registration REGISTRATIONS[MAX_REGISTERED_FDS];
static int REG_COUNT = 0;

void Register_init()
{
    memset(REGISTRATIONS, 0, sizeof(REGISTRATIONS));
}

void Register_connect(int fd, int conn_type)
{
    assert(fd < MAX_REGISTERED_FDS && "Attempt to register FD that's greater than 64k possible.");
    assert(conn_type != 0 && "conn_type can't be 0");

    Registration *reg = &REGISTRATIONS[fd];
    reg->conn_type = conn_type;
    reg->last_ping = time(NULL);
    REG_COUNT++;
}


void Register_disconnect(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "Attempt to register FD that's greater than 64k possible.");

    Registration *reg = &REGISTRATIONS[fd];
    reg->conn_type = 0;
    reg->last_ping = 0;
    fdclose(fd);
    REG_COUNT--;
}

int Register_ping(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "Attempt to register FD that's greater than 64k possible.");
    Registration *reg = &REGISTRATIONS[fd];

    if(reg->conn_type == 0) {
        return 0;
    } else {
        reg->last_ping = time(NULL);
        return reg->last_ping;
    }
}


int Register_exists(int fd)
{
    assert(fd < MAX_REGISTERED_FDS && "Attempt to register FD that's greater than 64k possible.");
    return REGISTRATIONS[fd].conn_type;
}

