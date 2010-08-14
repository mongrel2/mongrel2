#include <superpoll.h>
#include <mem/halloc.h>
#include <dbg.h>
#include <assert.h>
#include <sys/resource.h>
#include <unistd.h>
#include <assert.h>


static int MAXFD = 0;

enum {
    MAX_NOFILE = 1024 * 10
};

void SuperPoll_destroy(SuperPoll *sp)
{
    if(sp) {
        if(sp->idle_fd > 0) close(sp->idle_fd);
        if(sp->idle_active) {
            list_destroy_nodes(sp->idle_active);
            list_destroy(sp->idle_active);
        }

        if(sp->idle_free) {
            list_destroy_nodes(sp->idle_free);
            list_destroy(sp->idle_free);
        }

        h_free(sp);
    }
}


int SuperPoll_arm_idle_fd(SuperPoll *sp);
int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd);
int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw);
int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result);


SuperPoll *SuperPoll_create()
{
    SuperPoll *sp = h_calloc(sizeof(SuperPoll), 1);
    check_mem(sp);
   
    int total_open_fd = SuperPoll_get_max_fd(MAX_NOFILE);
    sp->max_hot = total_open_fd / 4;
    sp->nfd_hot = 0;

    sp->pollfd = h_calloc(sizeof(zmq_pollitem_t), sp->max_hot);
    check_mem(sp->pollfd);
    hattach(sp->pollfd, sp);

    sp->hot_data = h_calloc(sizeof(void *), sp->max_hot);
    check_mem(sp->hot_data);
    hattach(sp->hot_data, sp);

    check(SuperPoll_setup_idle(sp, total_open_fd) == 0, "Failed to configure epoll. Disabling.");

    log_info("Allowing for %d hot and %d idle file descriptors.", sp->max_hot, sp->max_idle);

    return sp;

error:
    SuperPoll_destroy(sp);

    return NULL;
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
        return SuperPoll_add_idle(sp, data, fd, rw);
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


int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms)
{
    int i = 0;
    int nfound = 0;
    int cur_i = 0;
    int rc = 0;
    int hit_idle = 0;

    result->nhits = 0;

    // do the regular poll, with idlefd inside
    nfound = zmq_poll(sp->pollfd, sp->nfd_hot, ms);
    check(nfound >= 0 || errno == EINTR, "zmq_poll failed.");

    result->hot_fds = nfound;

    for(i = 0; i < nfound; i++) {
        // TODO: change this to only scan then assert < nfd_hot
        while(cur_i < sp->nfd_hot && !sp->pollfd[cur_i].revents) {
            cur_i++;
        }

        if(sp->pollfd[cur_i].fd == sp->idle_fd) {
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

    result->hot_atr = sp->nfd_hot ? result->hot_fds / sp->nfd_hot * 100 : 0;

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


#ifdef NO_EPOLL

inline int SuperPoll_arm_idle_fd(SuperPoll *sp)
{
    assert(0 && "Should not get called.");
}

inline int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd)
{
    sp->max_idle = 0;
    sp->events = NULL;
    sp->idle_fd = -1;
    sp->idle_data = NULL;
    sp->idle_free = NULL;
    sp->idle_active = NULL;
    return 0;
}

inline int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw)
{
    return SuperPoll_add_poll(sp, data, NULL, fd, rw);
}

inline int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result)
{
    return 0;
}

#else

#include <sys/epoll.h>

#define SuperPoll_epoll_events(S) ((struct epoll_event *)(S))

inline int SuperPoll_arm_idle_fd(SuperPoll *sp)
{
    return SuperPoll_add(sp, NULL, NULL, sp->idle_fd, 'r', 1);
}

inline int SuperPoll_setup_idle(SuperPoll *sp, int total_open_fd) 
{
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
    for(i = 0; i < sp->max_idle; i++) {
        lnode_t *n = lnode_create(&sp->idle_data[i]);
        check_mem(n);
        list_append(sp->idle_free, n);
    }

    sp->idle_active = list_create(sp->max_idle);
    check_mem(sp->idle_active);

    int rc = SuperPoll_arm_idle_fd(sp);
    check(rc != -1, "Failed to add the epoll socket to the poll list.");

    return 0;
error:
    return -1;
}


inline int SuperPoll_add_idle(SuperPoll *sp, void *data, int fd, int rw)
{
    // take one off the free list, set it up, push onto the actives
    lnode_t *next = list_del_last(sp->idle_free);

    check(next, "Too many open files, no free idle slots.");
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


inline int SuperPoll_add_idle_hits(SuperPoll *sp, PollResult *result)
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

        assert(list_count(sp->idle_active) + list_count(sp->idle_free) == sp->max_idle && "We lost one somewhere.");
    }

    result->idle_fds = nfds;
    return nfds;

error:
    return -1;
}

#endif  // NO_EPOLL
