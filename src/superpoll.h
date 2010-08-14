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
