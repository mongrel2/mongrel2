#ifndef _superpoll_h
#define _superpoll_h

#include <adt/heap.h>
#include <zmq.h>

typedef struct SuperPoll {
    int max_idle;
    int max_hot;

    zmq_pollitem_t *pollfd;
    int npollfd;

    void **data;
} SuperPoll;


typedef struct PollEvent {
    zmq_pollitem_t poll;
    void *data;
} PollEvent;


typedef struct PollResult {
    int hot_fds;
    int hot_atr;
    PollEvent *hits;
} PollResult;

void SuperPoll_destroy(SuperPoll *sp);

SuperPoll *SuperPoll_create();

int SuperPoll_add(SuperPoll *sp, void *data, void *socket, int fd, int rw, int hot);

void SuperPoll_compact_down(SuperPoll *sp, int i);

int SuperPoll_poll(SuperPoll *sp, PollResult *result, int ms);

int SuperPoll_get_max_fd(int requested_max);

#define SuperPoll_active_count(S) ((S)->npollfd)

#define SuperPoll_max(S) ((S)->max_hot)

#define SuperPoll_data(S, I) ((S)->data[(I)])

#endif
