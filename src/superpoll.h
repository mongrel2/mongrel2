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

#ifndef _superpoll_h
#define _superpoll_h

#include <adt/list.h>
#include <zmq.h>

typedef struct IdleData {
    int fd;
    void *data;
} IdleData;

typedef struct SuperPoll {

    // poll information
    zmq_pollitem_t *pollfd;
    // caller's data
    void **hot_data;
    int nfd_hot;
    int max_hot;

    // idle information
    void *events;
    int idle_fd;

    int max_idle;
    IdleData *idle_data;
    list_t *idle_active;
    list_t *idle_free;
} SuperPoll;


typedef struct PollEvent {
    zmq_pollitem_t ev;
    void *data;
} PollEvent;


typedef struct PollResult {
    int hot_fds;
    int hot_atr;

    int idle_fds;
    int idle_atr;

    int nhits;
    PollEvent *hits;
} PollResult;

void SuperPoll_destroy(SuperPoll *sp);

SuperPoll *SuperPoll_create();

int SuperPoll_add(SuperPoll *sp, void *data, void *socket, int fd, int rw, int hot);

void SuperPoll_compact_down(SuperPoll *sp, int i);

int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms);

int SuperPoll_get_max_fd(int requested_max);

#define SuperPoll_active_count(S) ((S)->nfd_hot + list_count((S)->idle_active))

#define SuperPoll_max_hot(S) ((S)->max_hot)
#define SuperPoll_max_idle(S) ((S)->max_idle)

#define SuperPoll_data(S, I) ((S)->hot_data[(I)])

int PollResult_init(SuperPoll *p, PollResult *result);

void PollResult_clean(PollResult *result);

#endif
