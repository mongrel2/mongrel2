#include "taskimpl.h"
#include "dbg.h"

/*
 * locking
 */
static int _qlock(QLock *l, int block)
{
    if(l->owner == NULL){
        l->owner = taskrunning;
        return 1;
    }

    if(!block) {
        return 0;
    }

    addtask(&l->waiting, taskrunning);
    taskstate("qlock");
    taskswitch();

    assert(l->owner == taskrunning && "qlock error, owner is not the running task");

    return 1;
}

void qlock(QLock *l)
{
    _qlock(l, 1);
}

int canqlock(QLock *l)
{
    return _qlock(l, 0);
}

void qunlock(QLock *l)
{
    Task *ready;

    assert(l->owner != 0 && "qunlock: owner == 0\n");

    if((l->owner = ready = l->waiting.head) != NULL) {
        deltask(&l->waiting, ready);
        taskready(ready);
    }
}

static int _rlock(RWLock *l, int block)
{
    if(l->writer == NULL && l->wwaiting.head == NULL){
        l->readers++;
        return 1;
    }

    if(!block) {
        return 0;
    }

    addtask(&l->rwaiting, taskrunning);
    taskstate("rlock");
    taskswitch();
    
    return 1;
}

void rlock(RWLock *l)
{
    _rlock(l, 1);
}

int canrlock(RWLock *l)
{
    return _rlock(l, 0);
}

static int _wlock(RWLock *l, int block)
{
    if(l->writer == NULL && l->readers == 0) {
        l->writer = taskrunning;
        return 1;
    }

    if(!block) {
        return 0;
    }

    addtask(&l->wwaiting, taskrunning);
    taskstate("wlock");
    taskswitch();
    return 1;
}

void wlock(RWLock *l)
{
    _wlock(l, 1);
}

int canwlock(RWLock *l)
{
    return _wlock(l, 0);
}

void runlock(RWLock *l)
{
    Task *t;

    if(--l->readers == 0 && (t = l->wwaiting.head) != NULL) {
        deltask(&l->wwaiting, t);
        l->writer = t;
        taskready(t);
    }
}

void wunlock(RWLock *l)
{
    Task *t;
   
    assert(l->writer != NULL && "wunlock: not locked.");

    l->writer = NULL;
    assert(l->readers == 0 && "wunlock: readers is wrong.");

    while((t = l->rwaiting.head) != NULL){
        deltask(&l->rwaiting, t);
        l->readers++;
        taskready(t);
    }

    if(l->readers == 0 && (t = l->wwaiting.head) != NULL){
        deltask(&l->wwaiting, t);
        l->writer = t;
        taskready(t);
    }
}
