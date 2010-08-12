#include <superpoll.h>
#include <mem/halloc.h>
#include <dbg.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>

static int MAXFD = 0;

enum {
    MAX_NOFILE = 1024 * 10
};

void SuperPoll_destroy(SuperPoll *sp)
{
    if(sp) {
        close(sp->epoll_fd);
        h_free(sp);
    }
}

inline int SuperPoll_arm_epoll_fd(SuperPoll *sp)
{
    return SuperPoll_add(sp, NULL, NULL, sp->epoll_fd, 'r', 1);
}

SuperPoll *SuperPoll_create()
{
    SuperPoll *sp = h_calloc(sizeof(SuperPoll), 1);
    check_mem(sp);
   
    int total_open_fd = SuperPoll_get_max_fd(MAX_NOFILE);
    sp->max_hot = total_open_fd / 4;
    sp->max_idle = total_open_fd - sp->max_hot;
    sp->nfd_hot = 0;
    sp->nfd_idle = 0;

    log_info("Allowing for %d hot and %d idle file descriptors.", sp->max_hot, sp->max_idle);

    sp->pollfd = h_calloc(sizeof(zmq_pollitem_t), sp->max_hot);
    check_mem(sp->pollfd);
    hattach(sp->pollfd, sp);

    sp->hot_data = h_calloc(sizeof(void *), sp->max_hot);
    check_mem(sp->hot_data);
    hattach(sp->hot_data, sp);

    sp->idle_data = h_calloc(sizeof(void *), sp->max_idle);
    check_mem(sp->idle_data);
    hattach(sp->idle_data, sp);

    sp->events = h_calloc(sizeof(struct epoll_event), sp->max_idle);
    check_mem(sp->events);
    hattach(sp->events, sp);

	sp->epoll_fd = epoll_create(sp->max_idle);
    check(sp->epoll_fd != -1, "Failed to create the epoll structure.");

    int rc = SuperPoll_arm_epoll_fd(sp);
    check(rc != -1, "Failed to add the epoll socket to the poll list.");

    return sp;

error:
    SuperPoll_destroy(sp);

    return NULL;
}

struct bad_hack {
    uint32_t fd;
    uint32_t data_i;
};

inline int SuperPoll_add_epoll(SuperPoll *sp, void *data, int fd, int rw)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    sp->idle_data[sp->nfd_idle] = data;

    struct bad_hack bh = {.fd = (uint32_t)fd, .data_i = sp->nfd_idle};
    sp->nfd_idle++;

    memcpy(&event.data.u64, &bh, sizeof(bh));

    if(rw == 'r') {
        event.events = EPOLLIN;
    } else if(rw == 'w') {
        event.events = EPOLLOUT;
    } else {
        sentinel("Invalid event %c handed to superpoll.  r/w only.", rw);
    }

    int rc = epoll_ctl(sp->epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(rc == -1 && errno == EEXIST) {
        // that's already in there so do a mod instead
        rc = epoll_ctl(sp->epoll_fd, EPOLL_CTL_MOD, fd, &event);
        check(rc != -1, "Could not MOD fd that's already in epoll.");
    } else if(rc == -1) {
        sentinel("Failed to add FD to epoll.");
    } else {
        return sp->nfd_idle;
    }

error:
    return -1;
}


inline int SuperPoll_add_poll(SuperPoll *sp, void *data, void *socket, int fd, int rw)
{
    int cur_fd = sp->nfd_hot;
    int bits = 0;

    check(cur_fd < SuperPoll_max_hot(sp), "Too many open files requested: %d is greater than hot %d max.", cur_fd, SuperPoll_max_hot(sp));


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
    if(hot) {
        return SuperPoll_add_poll(sp, data, socket, fd, rw);
    } else {
        assert(!socket && "Cannot add a 0MQ socket to the idle (!hot) set.");
        return SuperPoll_add_epoll(sp, data, fd, rw);
    }
}


void SuperPoll_compact_down(SuperPoll *sp, int i)
{
    sp->nfd_hot--;
    sp->pollfd[i] = sp->pollfd[sp->nfd_hot];
    sp->hot_data[i] = sp->hot_data[sp->nfd_hot];
}

inline void SuperPoll_add_hit(PollResult *result, zmq_pollitem_t *p, void *data)
{
    result->hits[result->nhits].ev = *p;
    result->hits[result->nhits].data = data;
    result->nhits++;
}


inline int SuperPoll_add_epoll_hits(SuperPoll *sp, PollResult *result)
{
    int nfds = 0;
    int i = 0;
    int rc = 0;
    zmq_pollitem_t ev;
    struct bad_hack bh;

    nfds = epoll_wait(sp->epoll_fd, sp->events, sp->max_idle, -1);
    check(nfds >= 0, "Error doing epoll.");

    for(i = 0; i < nfds; i++) {
        memcpy(&bh, &(sp->events[i].data.u64), sizeof(bh));
        ev.fd = bh.fd;
        ev.revents = 0;

        if(sp->events[i].events & EPOLLIN) {
            ev.revents = ZMQ_POLLIN;
        }

        if(sp->events[i].events & EPOLLOUT) {
            ev.revents = ZMQ_POLLOUT;
        }

        if(ev.revents) {
            SuperPoll_add_hit(result, &ev, sp->idle_data[bh.data_i]);
        }

        // no matter what remove it
        rc = epoll_ctl(sp->epoll_fd, EPOLL_CTL_DEL, ev.fd, NULL);
        check(rc != -1, "Failed to remove fd %d from epoll.", ev.fd);

        sp->idle_data[bh.data_i] = NULL;
    }

    // TODO: this is totally fucked, but works for now
    while(sp->idle_data[sp->nfd_idle - 1] == NULL && sp->nfd_idle > 0) {
        sp->nfd_idle--;
    }

    result->idle_fds = nfds;
    return nfds;

error:
    return -1;
}


int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms)
{
    int i = 0;
    int nfound = 0;
    int cur_i = 0;


    result->nhits = 0;

    nfound = zmq_poll(sp->pollfd, sp->nfd_hot, ms);
    check(nfound >= 0 || errno == EINTR, "zmq_poll failed.");

    result->hot_fds = nfound;

    int hit_epoll = 0;

    for(i = 0; i < nfound; i++) {
        while(cur_i < sp->nfd_hot && !sp->pollfd[cur_i].revents) {
            cur_i++;
        }

        if(sp->pollfd[cur_i].fd == sp->epoll_fd) {
            hit_epoll = 1;
            SuperPoll_add_epoll_hits(sp, result);
        } else {
            SuperPoll_add_hit(result, &sp->pollfd[cur_i], sp->hot_data[cur_i]);
        }

        SuperPoll_compact_down(sp, cur_i);
    }

    if(hit_epoll) {
        SuperPoll_arm_epoll_fd(sp);
    }

    result->hot_atr = sp->nfd_hot ? result->hot_fds / sp->nfd_hot * 100 : 0;
    result->idle_atr = sp->nfd_idle ? result->idle_fds / sp->nfd_idle * 100 : 0;

    if(result->idle_atr < 100 && result->idle_atr > 1) {
        debug("HOT ATR: %d IDLE ATR: %d", result->hot_atr, result->idle_atr);
    }
    return result->nhits;

error:
    return -1;

}

int SuperPoll_get_max_fd(int requested_max)
{
    int rc = 0;
    struct rlimit rl;

    if(!requested_max) requested_max = MAX_NOFILE;

    if(MAXFD) return MAXFD;

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

    log_info("MAX open file descriptors is %d now.", MAXFD);
    return MAXFD;

error:
    log_err("TOTAL CATASTROPHE, if this happens we can't get your rlimit for max files, picking 256 to be safe.");

    MAXFD = 256;
    return MAXFD;
}


int PollResult_init(SuperPoll *p, PollResult *result)
{
    memset(result, 0, sizeof(PollResult));
    result->hits = calloc(sizeof(PollEvent), SuperPoll_max_hot(p) + SuperPoll_max_idle(p));
    check_mem(result->hits);

    return 0;

error:
    return -1;
}

void PollResult_clean(PollResult *result)
{
    if(result) {
        if(result->hits) free(result->hits);
    }
}
