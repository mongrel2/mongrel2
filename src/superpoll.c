#include <superpoll.h>
#include <mem/halloc.h>
#include <dbg.h>
#include <assert.h>
#include <sys/resource.h>

static int MAXFD = 0;

enum {
    MAX_NOFILE = 1024 * 10
};

void SuperPoll_destroy(SuperPoll *sp)
{
    if(sp) h_free(sp);
}

SuperPoll *SuperPoll_create()
{
    SuperPoll *sp = h_calloc(sizeof(SuperPoll), 1);
    check_mem(sp);
    
    sp->max_idle = 0; // this will be for later when we start dealing with idles
    sp->max_hot = SuperPoll_get_max_fd(MAX_NOFILE);

    sp->pollfd = h_calloc(sizeof(zmq_pollitem_t), sp->max_hot);
    check_mem(sp->pollfd);
    hattach(sp, sp->pollfd);

    sp->data = h_calloc(sizeof(void *), sp->max_hot + sp->max_idle);
    check_mem(sp->data);
    hattach(sp, sp->data);

    return sp;

error:
    SuperPoll_destroy(sp);

    return NULL;
}


int SuperPoll_add(SuperPoll *sp, void *data, void *socket, int fd, int rw, int hot)
{
    if(sp->npollfd >= SuperPoll_max(sp)){
        errno = EBUSY;
        return -1;
    }

    int bits = 0;
    if(rw == 'r') {
        bits |= ZMQ_POLLIN;
    } else if(rw == 'w') {
        bits |= ZMQ_POLLOUT;
    } else {
        sentinel("Invalid rw option '%c'", rw);
    }

    sp->pollfd[sp->npollfd].fd = fd;
    sp->pollfd[sp->npollfd].socket = socket;
    sp->pollfd[sp->npollfd].events = bits;
    sp->pollfd[sp->npollfd].revents = 0;
    sp->data[sp->npollfd] = data;
    sp->npollfd++;

    return sp->npollfd;

error:
    return -1;
}



void SuperPoll_compact_down(SuperPoll *sp, int i)
{
    sp->npollfd--;
    sp->pollfd[i] = sp->pollfd[sp->npollfd];
    sp->data[i] = sp->data[sp->npollfd];
}

inline void SuperPoll_add_hit(PollResult *result, int i, zmq_pollitem_t *p, void *data)
{
    assert(i < result->hot_fds && "Error, added more hits than possible, tell Zed.");
    result->hits[i].ev = *p;
    result->hits[i].data = data;
}


int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms)
{
    int i = 0;
    int nfound = 0;
    int cur_i = 0;

    nfound = zmq_poll(sp->pollfd, sp->npollfd, ms);
    check(nfound >= 0 || errno == EINTR, "zmq_poll failed.");

    result->hot_fds = nfound; 

    if(sp->npollfd - 2) {
        result->hot_atr = 1000 * nfound / sp->npollfd;
    }

    for(i = 0; i < nfound; i++) {
        while(cur_i < sp->npollfd && !sp->pollfd[cur_i].revents) {
            cur_i++;
        }

        SuperPoll_add_hit(result, i, &sp->pollfd[cur_i], sp->data[cur_i]);
        SuperPoll_compact_down(sp, cur_i);
    }

    return result->hot_fds;

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
    result->hits = calloc(sizeof(PollEvent), SuperPoll_max(p));
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
