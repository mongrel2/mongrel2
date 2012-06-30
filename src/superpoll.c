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


#include <superpoll.h>
#include <mem/halloc.h>
#include <dbg.h>
#include <assert.h>
#include <sys/resource.h>
#include <unistd.h>
#include <assert.h>
#include <setting.h>

#ifdef __linux__
#define HAS_EPOLL 1
#else
#define HAS_EPOLL 0
#endif

static int MAXFD = 0;

enum {
    MAX_NOFILE = 1024 * 10
};

void SuperPoll_destroy(SuperPoll *sp)
{
    if(sp) {
        if(HAS_EPOLL) {
            if(sp->idle_fd > 0) close(sp->idle_fd);
            if(sp->idle_active) {
                list_destroy_nodes(sp->idle_active);
                list_destroy(sp->idle_active);
            }

            if(sp->idle_free) {
                list_destroy_nodes(sp->idle_free);
                list_destroy(sp->idle_free);
            }
        }

        h_free(sp);
    }
}


static inline int SuperPoll_arm_idle_fd(SuperPoll *sp);
static inline int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd);
static inline int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw);
static inline int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result);


SuperPoll *SuperPoll_create()
{
    SuperPoll *sp = h_calloc(sizeof(SuperPoll), 1);
    check_mem(sp);
    int rc = 0;

    int total_open_fd = SuperPoll_get_max_fd();
    sp->nfd_hot = 0;

    if(HAS_EPOLL) {
        int hot_dividend = Setting_get_int("superpoll.hot_dividend", 4);

        sp->max_hot = total_open_fd / hot_dividend;

        rc = SuperPoll_setup_idle(sp, total_open_fd);
        check(rc == 0, "Failed to configure epoll. Disabling.");

        debug("Allowing for %d hot and %d idle file descriptors (dividend was %d)",
                sp->max_hot, sp->max_idle, hot_dividend);
    } else {
        sp->max_hot = total_open_fd;

        debug("You do not have epoll support. Allowing for %d file descriptors through poll.", sp->max_hot);
    }

    sp->pollfd = h_calloc(sizeof(zmq_pollitem_t), sp->max_hot);
    check_mem(sp->pollfd);
    hattach(sp->pollfd, sp);

    sp->hot_data = h_calloc(sizeof(void *), sp->max_hot);
    check_mem(sp->hot_data);
    hattach(sp->hot_data, sp);

    if(HAS_EPOLL) {
        int rc = SuperPoll_arm_idle_fd(sp);
        check(rc != -1, "Failed to add the epoll socket to the poll list.");
    }

    return sp;

error:
    SuperPoll_destroy(sp);

    return NULL;
}



static inline int SuperPoll_add_poll(SuperPoll *sp, void *data, void *socket, int fd, int rw)
{
    int cur_fd = sp->nfd_hot;
    int bits = 0;

    check(socket != NULL || fd >= 0, "Attempt to %s from dead file descriptor: %d", rw == 'r' ? "read" : "write", fd);
    check(cur_fd < SuperPoll_max_hot(sp), "Too many %s: %d is greater than hot %d max.",
            socket ? "handler requests outstanding, your handler isn't running" : "files open",
            cur_fd, SuperPoll_max_hot(sp));


    if(rw == 'r') {
        bits |= ZMQ_POLLIN;
    } else if(rw == 'w') {
        bits |= ZMQ_POLLOUT;
    } else {
        sentinel("Invalid event %c handed to superpoll.  r/w only.", rw);
    }

    sp->pollfd[cur_fd].fd = fd;
    sp->pollfd[cur_fd].socket = socket;
    sp->pollfd[cur_fd].events = bits;
    sp->pollfd[cur_fd].revents = 0;
    sp->hot_data[cur_fd] = data;
    sp->nfd_hot++;

    return sp->nfd_hot;

error:
    return -1;
}

int SuperPoll_add(SuperPoll *sp, void *data, void *socket, int fd, int rw, int hot)
{
    if(socket || hot || !HAS_EPOLL) {
        return SuperPoll_add_poll(sp, data, socket, fd, rw);
    } else {
        assert(!socket && "Cannot add a 0MQ socket to the idle (!hot) set.");
        return SuperPoll_add_idle(sp, data, fd, rw);
    }
}

int SuperPoll_del(SuperPoll *sp, void *socket, int fd, int hot)
{
    int i = 0;
    int slot = 0;

    for(i = 0; i < sp->nfd_hot; i++) {
        if(socket) {
            if(sp->pollfd[i].socket == socket) {
                slot = i; break;
            }
        } else if(hot) {
            if(sp->pollfd[i].fd == fd) {
                slot = i; break;
            }
        } else {
            sentinel("Not implemented yet.");
        }
    }

    SuperPoll_compact_down(sp, slot);

    return 0;
error:
    return -1;
}


void SuperPoll_compact_down(SuperPoll *sp, int i)
{
    sp->nfd_hot--;
    if(sp->nfd_hot >= 0) {
        sp->pollfd[i] = sp->pollfd[sp->nfd_hot];
        sp->hot_data[i] = sp->hot_data[sp->nfd_hot];
    }
}

static inline void SuperPoll_add_hit(PollResult *result, zmq_pollitem_t *p, void *data)
{
    result->hits[result->nhits].ev = *p;
    result->hits[result->nhits].data = data;
    result->nhits++;
}


int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms)
{
    int i = 0;
    int nfound = 0;
    int cur_i = 0;
    int rc = 0;
    int hit_idle = 0;

    result->nhits = 0;

    // do the regular poll, with idlefd inside if available; 0MQ 3.1 poll is in ms.
    nfound = zmq_poll(sp->pollfd, sp->nfd_hot, ms * ZMQ_POLL_MSEC);
    check(nfound >= 0 || errno == EINTR, "zmq_poll failed.");

    result->hot_fds = nfound;

    for(i = 0; i < nfound; i++) {
        while(cur_i < sp->nfd_hot && !sp->pollfd[cur_i].revents) {
            cur_i++;
        }

        if(HAS_EPOLL && sp->pollfd[cur_i].fd == sp->idle_fd) {
            hit_idle = 1;
            rc = SuperPoll_add_idle_hits(sp, result);
            check(rc != -1, "Failed to add idle hits.");
        } else {
            SuperPoll_add_hit(result, &sp->pollfd[cur_i], sp->hot_data[cur_i]);
        }

        SuperPoll_compact_down(sp, cur_i);
    }

    if(hit_idle) {
        SuperPoll_arm_idle_fd(sp);
    }

    return result->nhits;

error:
    return -1;

}

int SuperPoll_get_max_fd()
{
    int rc = 0;
    struct rlimit rl;

    if(MAXFD) return MAXFD;

    int requested_max = Setting_get_int("superpoll.max_fd", MAX_NOFILE);

    debug("Attempting to force NOFILE limit to %d", requested_max);
    rl.rlim_cur = requested_max;
    rl.rlim_max = requested_max;

    rc = setrlimit(RLIMIT_NOFILE, &rl);

    if(rc == 0) {
        MAXFD = requested_max;
    } else {
        log_info("Could not force NOFILE higher, you'll need to run as root: %s", strerror(errno));
        rc = getrlimit(RLIMIT_NOFILE, &rl);
        check(rc == 0, "Failed to get your max open file limit, totally weird.");
        MAXFD = rl.rlim_cur;
    }

    debug("MAX open file descriptors is %d now.", MAXFD);
    return MAXFD;

error:
    log_err("TOTAL CATASTROPHE, if this happens we can't get your rlimit for max files, picking 256 to be safe.");

    MAXFD = 256;
    return MAXFD;
}


int PollResult_init(SuperPoll *p, PollResult *result)
{
    memset(result, 0, sizeof(PollResult));
    result->hits = h_calloc(sizeof(PollEvent), SuperPoll_max_hot(p) + SuperPoll_max_idle(p));
    hattach(result->hits, p);
    check_mem(result->hits);

    return 0;

error:
    return -1;
}

void PollResult_clean(PollResult *result)
{
    if(result) {
        if(result->hits) h_free(result->hits);
    }
}


#if defined HAS_EPOLL && HAS_EPOLL == 0

static inline int SuperPoll_arm_idle_fd(SuperPoll *sp)
{
    assert(0 && "Should not get called.");
    return -1;
}

static inline int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd)
{
    sp->max_idle = 0;
    sp->events = NULL;
    sp->idle_fd = -1;
    sp->idle_data = NULL;
    sp->idle_free = NULL;
    sp->idle_active = NULL;
    return 0;
}

static inline int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw)
{
    return SuperPoll_add_poll(sp, data, NULL, fd, rw);
}

static inline int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result)
{
    return 0;
}

#else

#include <sys/epoll.h>

#define SuperPoll_epoll_events(S) ((struct epoll_event *)(S->events))

static inline int SuperPoll_arm_idle_fd(SuperPoll *sp)
{
    return SuperPoll_add(sp, NULL, NULL, sp->idle_fd, 'r', 1);
}

static inline int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd) 
{
    assert(HAS_EPOLL == 1 && "This function should not run unless HAS_EPOLL is 1.");

    int i = 0;

    sp->max_idle = total_open_fd - sp->max_hot;
    assert(sp->max_idle >= 0 && "max idle is can't be less than zero.");

    // setup the stuff for the epoll
    sp->events = h_calloc(sizeof(struct epoll_event), sp->max_idle);
    check_mem(sp->events);
    hattach(sp->events, sp);

	sp->idle_fd = epoll_create(sp->max_idle);
    check(sp->idle_fd != -1, "Failed to create the epoll structure.");

    sp->idle_data = h_calloc(sizeof(IdleData), sp->max_idle);
    check_mem(sp->idle_data);
    hattach(sp->idle_data, sp);

    sp->idle_free = list_create(sp->max_idle);
    check_mem(sp->idle_free);

    // load the free list to prime the pump for later usage
    debug("Building up slots for %d sockets in idle. Could take a minute.", sp->max_idle);
    for(i = 0; i < sp->max_idle; i++) {
        lnode_t *n = lnode_create(&sp->idle_data[i]);
        check_mem(n);
        list_append(sp->idle_free, n);
    }

    sp->idle_active = list_create(sp->max_idle);
    check_mem(sp->idle_active);

    return 0;
error:
    return -1;
}


static inline int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw)
{
    // take one off the free list, set it up, push onto the actives
    check(!list_isempty(sp->idle_free), "Too many open files, no free idle slots.");

    lnode_t *next = list_del_last(sp->idle_free);

    IdleData *id = (IdleData *)lnode_get(next);
    assert(id && "Got an id NULL ptr from the free list.");

    id->fd = fd;
    id->data = data;

    list_append(sp->idle_active, next);

    // hook up the epoll event for our epoll call
    struct epoll_event event;

    if(rw == 'r') {
        struct epoll_event ev = {.data = {.ptr = next}, .events = EPOLLIN | EPOLLONESHOT};
        event = ev;
    } else if(rw == 'w') {
        struct epoll_event ev = {.data = {.ptr = next}, .events = EPOLLOUT | EPOLLONESHOT};
        event = ev;
    } else {
        sentinel("Invalid event %c handed to superpoll.  r/w only.", rw);
    }

    // do the actual epoll call, but if we get that it's in there then mod it
    int rc = epoll_ctl(sp->idle_fd, EPOLL_CTL_ADD, fd, &event);

    if(rc == -1 && errno == EEXIST) {
        // that's already in there so do a mod instead
        rc = epoll_ctl(sp->idle_fd, EPOLL_CTL_MOD, fd, &event);
        check(rc != -1, "Could not MOD fd that's already in epoll.");
    } else if(rc == -1) {
        sentinel("Failed to add FD to epoll.");
    } else {
        return 1;
    }

error:
    return -1;
}


static inline int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result)
{
    int nfds = 0;
    int i = 0;
    int rc = 0;
    zmq_pollitem_t ev = {.socket = NULL};

    nfds = epoll_wait(sp->idle_fd, SuperPoll_epoll_events(sp), sp->max_idle, 0);
    check(nfds >= 0, "Error doing epoll.");
    struct epoll_event *events = SuperPoll_epoll_events(sp);

    for(i = 0; i < nfds; i++) {
        lnode_t *node = (lnode_t *)events[i].data.ptr;
        IdleData *data = lnode_get(node);
        ev.fd = data->fd;

        if(events[i].events & EPOLLIN) {
            ev.revents = ZMQ_POLLIN;
        }

        if(events[i].events & EPOLLOUT) {
            ev.revents = ZMQ_POLLOUT;
        }

        if(ev.revents) {
            SuperPoll_add_hit(result, &ev, data->data);
        }

        // no matter what remove it
        rc = epoll_ctl(sp->idle_fd, EPOLL_CTL_DEL, ev.fd, NULL);
        check(rc != -1, "Failed to remove fd %d from epoll.", ev.fd);

        // take it out of active and put into free list
        node = list_delete(sp->idle_active, node);
        list_append(sp->idle_free, node);

        assert(list_count(sp->idle_active) + list_count(sp->idle_free) == (int)sp->max_idle && "We lost one somewhere.");
    }

    result->idle_fds = nfds;
    return nfds;

error:
    return -1;
}

#endif  // HAS_EPOLL
