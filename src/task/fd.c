#include "taskimpl.h"
#include <zmq.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <superpoll.h>
#include <dbg.h>
#include "setting.h"


static int STARTED_FDTASK = 0;
static Tasklist sleeping;
static int sleepingcounted;
static uvlong nsec(void);
SuperPoll *POLL = NULL;

void *ZMQ_CTX = NULL;

int FDSTACK= 100 * 1024;

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif


void mqinit(int threads)
{
    if(ZMQ_CTX == NULL) {
        ZMQ_CTX = zmq_init(threads);

        if(!ZMQ_CTX) {
            printf("Error setting up 0mq.\n");
            exit(1);
        }
    }
}

static inline int next_task_sleeptime(int min)
{
    Task *t;
    uvlong now = 0;
    int ms = 0;

    if((t=sleeping.head) == NULL)
        ms = -1;
    else{
        now = nsec();
        if(now >= t->alarmtime) {
            ms = 0;
        } else {
            ms = (t->alarmtime - now) / 1000000;
        }

        if(ms % min != 0) ms = ms - (ms % min);
    }

    if(ms == 0) ms = min;

    return ms;
}

static inline void wake_sleepers()
{
    Task *t;
    uvlong now = nsec();

    while((t =sleeping.head) && now >= t->alarmtime){
        deltask(&sleeping, t);

        if(!t->system && --sleepingcounted == 0) taskcount--;

        taskready(t);
    }
}

void
fdtask(void *v)
{
    int i, ms;
    PollResult result;
    int rc = 0;
    
    rc = PollResult_init(POLL, &result);
    check(rc == 0, "Failed to initialize the poll result.");

    tasksystem();
    taskname("fdtask");

    for(;;){
        /* let everyone else run */
        while(taskyield() > 0)
            ;
        /* we're the only one runnable - poll for i/o */
        errno = 0;
        taskstate("poll");

        ms = next_task_sleeptime(500);

        rc = SuperPoll_poll(POLL, &result, ms);
        check(rc != -1, "SuperPoll failure, aborting.");

        for(i = 0; i < rc; i++) {
            taskready(result.hits[i].data); 
        }

        wake_sleepers();
    }

    PollResult_clean(&result);
    taskexit(0);

error:
    taskexitall(1);
}


int tasknuke(int id)
{
    int i = 0;
    Task *target = NULL;

    for(i = 0; i < SuperPoll_active_hot(POLL); i++) {
        target = (Task *)SuperPoll_data(POLL, i);

        if(target && target->id == id) {
            SuperPoll_compact_down(POLL, i);
            return 0;
        }
    }

    return -1;
}

static inline void startfdtask()
{
    if(!STARTED_FDTASK) {
        FDSTACK = Setting_get_int("limits.fdtask_stack", 100 * 1024);
        log_info("MAX limits.fdtask_stack=%d", FDSTACK);

        POLL = SuperPoll_create();
        STARTED_FDTASK = 1;
        taskcreate(fdtask, 0, FDSTACK);
    }
}

int taskwaiting()
{
    startfdtask();

    // TODO: do this right, for now just -1 for the epoll
    return SuperPoll_active_count(POLL) - 1;
}


uint taskdelay(uint ms)
{
    uvlong when = 0L;
    uvlong now = 0L;
    Task *t = NULL;
   
    startfdtask();

    now = nsec();
    when = now + (uvlong)ms * 1000000;
    for(t=sleeping.head; t != NULL && t->alarmtime < when; t=t->next)
        ;

    if(t) {
        taskrunning->prev = t->prev;
        taskrunning->next = t;
    } else {
        taskrunning->prev = sleeping.tail;
        taskrunning->next = NULL;
    }
    
    t = taskrunning;
    t->alarmtime = when;

    if(t->prev) {
        t->prev->next = t;
    } else {
        sleeping.head = t;
    }

    if(t->next) {
        t->next->prev = t;
    } else {
        sleeping.tail = t;
    }

    if(!t->system && sleepingcounted++ == 0) {
        taskcount++;
    }

    taskswitch();

    return (nsec() - now) / 1000000;
}

int _wait(void *socket, int fd, int rw)
{
    startfdtask();

    int max = 0;
    int hot_add = SuperPoll_active_hot(POLL) < SuperPoll_max_hot(POLL);
    
    taskstate(rw == 'r' ? "read wait" : "write wait");

    max = SuperPoll_add(POLL, (void *)taskrunning, socket, fd, rw, hot_add);
    check(max != -1, "Error adding fd %d to task wait list.", fd);

    taskswitch();
    return 0;

error:
    taskswitch();
    return -1;
}

int fdwait(int fd, int rw)
{
    return _wait(NULL, fd, rw);
}

void *mqsocket(int type)
{
    return zmq_socket(ZMQ_CTX, type);
}

int mqwait(void *socket, int rw)
{
    return _wait(socket, -1, rw);
}

int mqrecv(void *socket, zmq_msg_t *msg, int flags)
{
    int rc = -1;

    // try to send right away rather than go into the poll
    rc = zmq_recv(socket, msg, flags);

    // if the send failed because it would block, then wait
    if(rc != 0 && flags == ZMQ_NOBLOCK && errno == EAGAIN) {
        if(mqwait(socket, 'r') == -1) {
            return -1;
        }

        rc = zmq_recv(socket, msg, flags);
    }

    return rc;
}

int mqsend(void *socket, zmq_msg_t *msg, int flags)
{
    int rc = -1;

    // try to send right away rather than go into the poll
    rc = zmq_send(socket, msg, flags);

    // if the send failed because it would block, then wait
    if(rc != 0 && flags == ZMQ_NOBLOCK && errno == EAGAIN) {
        if(mqwait(socket, 'w') == -1) {
            return -1;
        }

        rc = zmq_send(socket, msg, flags);
    }

    return rc;
}


/* Like fdread but always calls fdwait before reading. */
int fdread1(int fd, void *buf, int n)
{
    int m;
    
    do {
        if(fdwait(fd, 'r') == -1) {
            return -1;
        }
    } while((m = read(fd, buf, n)) < 0 && errno == EAGAIN);

    return m;
}

int fdrecv1(int fd, void *buf, int n)
{
    int m;
    
    do {
        if(fdwait(fd, 'r') == -1) {
            return -1;
        }
    } while((m = recv(fd, buf, n, MSG_NOSIGNAL)) < 0 && errno == EAGAIN);

    return m;
}

int fdread(int fd, void *buf, int n)
{
    int m;

    while((m=read(fd, buf, n)) < 0 && errno == EAGAIN) {
        if(fdwait(fd, 'r') == -1) {
            return -1;
        }
    }
    return m;
}

int fdrecv(int fd, void *buf, int n)
{
    int m;

    while((m=recv(fd, buf, n, MSG_NOSIGNAL)) < 0 && errno == EAGAIN) {
        if(fdwait(fd, 'r') == -1) {
            return -1;
        }
    }
    return m;
}

int fdwrite(int fd, void *buf, int n)
{
    int m = 0;
    int tot = 0;
    
    for(tot = 0; tot < n; tot += m){
        while((m=write(fd, (char*)buf+tot, n-tot)) < 0 && errno == EAGAIN) {
            if(fdwait(fd, 'w') == -1) {
                return -1;
            }
        }

        if(m < 0) return m;
        if(m == 0) break;
    }

    return tot;
}

int fdsend(int fd, void *buf, int n)
{
    int m = 0;
    int tot = 0;
    
    for(tot = 0; tot < n; tot += m){
        while((m=send(fd, (char*)buf+tot, n-tot, MSG_NOSIGNAL)) < 0 && errno == EAGAIN) {
            if(fdwait(fd, 'w') == -1) {
                return -1;
            }
        }

        if(m < 0) return m;
        if(m == 0) break;
    }
    return tot;
}

int fdnoblock(int fd)
{
#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
}

static uvlong nsec(void)
{
    struct timeval tv;

    if(gettimeofday(&tv, 0) < 0) {
        return -1;
    }

    return (uvlong)tv.tv_sec*1000*1000*1000 + tv.tv_usec*1000;
}


