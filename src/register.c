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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "tnetstrings.h"
#include "tnetstrings_impl.h"

#include "register.h"
#include "connection.h"
#include "dbg.h"
#include "task/task.h"
#include "adt/darray.h"
#include "setting.h"
#include "adt/radixmap.h"

uint32_t THE_CURRENT_TIME_IS = 0;

static darray_t *REGISTRATIONS = NULL;
static RadixMap *REG_ID_TO_FD = NULL;
static int NUM_REG_FD = 0;

void Register_init()
{
    THE_CURRENT_TIME_IS = time(NULL);
    REGISTRATIONS = darray_create(sizeof(Registration), MAX_REGISTERED_FDS);
    REG_ID_TO_FD = RadixMap_create(MAX_REGISTERED_FDS * 10);
}


void Register_destroy()
{
    darray_destroy(REGISTRATIONS);
    RadixMap_destroy(REG_ID_TO_FD);
}

static inline void Register_clear(Registration *reg)
{
    reg->data = NULL;
    reg->last_ping = 0;
    reg->bytes_read = 0;
    reg->bytes_written = 0;
    reg->last_read = 0;
    reg->last_write = 0;

    if(reg->id != UINT32_MAX) {
        RMElement *el = RadixMap_find(REG_ID_TO_FD, reg->id);

        if(el != NULL) {
            RadixMap_delete(REG_ID_TO_FD, el);
        }
    }
}

int Register_connect(int fd, Connection* data)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(data != NULL, "data can't be NULL");

    Registration *reg = darray_get(REGISTRATIONS, fd);

    if(reg == NULL) {
        reg = darray_new(REGISTRATIONS);
        check(reg != NULL, "Failed to allocate a new registration.");

        // we only set this here since they stay in list forever rather than recycle
        darray_set(REGISTRATIONS, fd, reg);
        darray_attach(REGISTRATIONS, reg);
    }

    if(Register_valid(reg)) {
        // force them to exit
        int rc = Register_disconnect(fd);
        check(rc != -1, "Weird error trying to disconnect. Tell Zed.");
        tasksignal(reg->task, SIGINT);
    }

    reg->data = data;
    reg->last_ping = THE_CURRENT_TIME_IS;
    reg->fd = fd;
    reg->task = taskself();

    reg->id = UINT32_MAX; // start off with an invalid conn_id
    
    // keep track of the number of registered things we're tracking
    NUM_REG_FD++;

    return 0;
error:
    return -1;
}


int Register_disconnect(int fd)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for disconnect.");

    Registration *reg = darray_get(REGISTRATIONS, fd);

    check_debug(Register_valid(reg), "Attempt to unregister FD %d which is already gone.", fd);
    check(reg->fd == fd, "Asked to disconnect fd %d but register had %d", fd, reg->fd);

    if(IOBuf_close(reg->data->iob) != 0) {
        debug("Failed to close IOBuffer, probably SSL error.");
    }

    Register_clear(reg);

    // tracking the number of things we're processing
    NUM_REG_FD--;
    return 0;

error:
    fdclose(fd);
    return -1;
}

int Register_ping(int fd)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for ping: %d", fd);
    Registration *reg = darray_get(REGISTRATIONS, fd);

    check_debug(Register_valid(reg), "Attempt to ping an FD that isn't registered: %d", fd);

    reg->last_ping = THE_CURRENT_TIME_IS;
    return reg->last_ping;

error:
    return -1;
}


int Register_read(int fd, off_t bytes)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for Register_read: %d", fd);
    Registration *reg = darray_get(REGISTRATIONS, fd);

    if(Register_valid(reg)) {
        reg->last_read = THE_CURRENT_TIME_IS;
        reg->bytes_read += bytes;
        return reg->last_read;
    }

error: // fallthrough
    return 0;
}


int Register_write(int fd, off_t bytes)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for Register_write: %d", fd);
    Registration *reg = darray_get(REGISTRATIONS, fd);

    if(Register_valid(reg)) {
        reg->last_write = THE_CURRENT_TIME_IS;
        reg->bytes_written += bytes;
        return reg->last_write;
    }

error: // fallthrough
    return 0;
}


Connection *Register_fd_exists(int fd)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    check(fd >= 0, "Invalid FD given for exists check");

    Registration *reg = darray_get(REGISTRATIONS, fd);

    return reg != NULL ? reg->data : NULL;
error:
    return NULL;
}


int Register_fd_for_id(uint32_t id)
{
    RMElement *el = RadixMap_find(REG_ID_TO_FD, id);

    check_debug(el != NULL, "Id %d not registered.", id);

    Registration *reg = darray_get(REGISTRATIONS, el->data.value);
    check_debug(Register_valid(reg), "Nothing registered under id %d.", id);

    return reg->fd;
error:
    return -1;
}

uint32_t Register_id_for_fd(int fd)
{
    check(fd < MAX_REGISTERED_FDS, "FD given to register is greater than max.");
    Registration *reg = darray_get(REGISTRATIONS, fd);
    check_debug(Register_valid(reg), "No ID for fd: %d", fd);

    if(reg->id == UINT32_MAX) {
        // lazy load the conn_id since we don't always need it
        reg->id = RadixMap_push(REG_ID_TO_FD, reg->fd);
        check(reg->id != UINT32_MAX, "Failed to register new conn_id.");
    }

    return reg->id;
error:
    return -1;
}

#define ZERO_OR_DELTA(N, T) (T == 0 ? T : N - T)


struct tagbstring REGISTER_HEADERS = bsStatic("86:2:id,2:fd,4:type,9:last_ping,9:last_read,10:last_write,10:bytes_read,13:bytes_written,]");

tns_value_t *Register_info()
{
    int i = 0;
    Registration *reg;
    tns_value_t *rows = tns_new_list();
    int nscanned = 0;

    time_t now = THE_CURRENT_TIME_IS;

    for(i = 0, nscanned = 0; i < darray_max(REGISTRATIONS) && nscanned < NUM_REG_FD; i++) {
        reg = darray_get(REGISTRATIONS, i);

        if(Register_valid(reg)) {
            nscanned++;  // stop scaning after we found all of them

            tns_value_t *data = tns_new_list();
            tns_add_to_list(data, tns_new_integer(reg->id == UINT32_MAX ? -1 : (long)reg->id));
            tns_add_to_list(data, tns_new_integer(i)); // fd
            tns_add_to_list(data, tns_new_integer(reg->data->type));
            tns_add_to_list(data, tns_new_integer(ZERO_OR_DELTA(now, reg->last_ping)));
            tns_add_to_list(data, tns_new_integer(ZERO_OR_DELTA(now, reg->last_read)));
            tns_add_to_list(data, tns_new_integer(ZERO_OR_DELTA(now, reg->last_write)));
            tns_add_to_list(data, tns_new_integer(reg->bytes_read));
            tns_add_to_list(data, tns_new_integer(reg->bytes_written));
            tns_add_to_list(rows, data);
        }
    }

    return tns_standard_table(&REGISTER_HEADERS, rows);
}

int Register_cleanout()
{
    int i = 0;
    int nkilled = 0;
    int nscanned = 0;
    time_t now = THE_CURRENT_TIME_IS;
    int min_ping = Setting_get_int("limits.min_ping", DEFAULT_MIN_PING);
    int min_write_rate = Setting_get_int("limits.min_write_rate", DEFAULT_MIN_READ_RATE);
    int min_read_rate = Setting_get_int("limits.min_read_rate", DEFAULT_MIN_WRITE_RATE);
    int kill_limit = Setting_get_int("limits.kill_limit", DEFAULT_KILL_LIMIT);

    for(i = 0, nscanned = 0; i < darray_max(REGISTRATIONS) && nscanned < NUM_REG_FD; i++) {
        Registration *reg = darray_get(REGISTRATIONS, i);

        if(Register_valid(reg)) {
            nscanned++; // avoid scanning the whole array if we've found them all

            int last_ping = ZERO_OR_DELTA(now, reg->last_ping);
            off_t read_rate = reg->bytes_read / (ZERO_OR_DELTA(now, reg->last_read) + 1);
            off_t write_rate = reg->bytes_written / (ZERO_OR_DELTA(now, reg->last_write) + 1);
            int should_kill = 0;

            debug("Checking fd=%d:conn_id=%d against last_ping: %d, read_rate: %d, write_rate: %d",
                    i, reg->id, last_ping, read_rate, write_rate);

            // these are weighted so they are not if-else statements
            if(min_ping != 0 && last_ping > min_ping) {
                debug("Connection fd=%d:conn_id=%d over limits.min_ping time: %d < %d",
                        i, reg->id, min_ping, last_ping);
                should_kill++;
            }
            
            if(min_read_rate != 0 && read_rate < min_read_rate) {
                debug("Connection fd=%d:conn_id=%d read rate lower than allowed: %d < %d",
                        i, reg->id, read_rate, min_read_rate);
                should_kill++;
            } 

            if(min_write_rate != 0 && write_rate < min_write_rate) {
                debug("Connection fd=%d:conn_id=%d write rate lower than allowed: %d < %d",
                        i, reg->id, write_rate, min_write_rate);
                should_kill++;
            }

            if(should_kill > kill_limit) {
                nkilled++;
                Register_disconnect(i);
            }
        }
    }

    if(nkilled) {
        log_warn("Killed %d connections according to min_ping: %d, min_write_rate: %d, min_read_rate: %d", nkilled, min_ping, min_write_rate, min_read_rate);
    }

    return nkilled;
}
