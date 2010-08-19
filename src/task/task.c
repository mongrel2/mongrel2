/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include "taskimpl.h"
#include <fcntl.h>
#include <stdio.h>

int    taskcount;
int    tasknswitch;
int    taskexitval;
Task    *taskrunning;

Context    taskschedcontext;
Tasklist    taskrunqueue;

enum {
    TASK_LIST_GROWTH=256
};

Task    **alltask;
int        nalltask;

static char *argv0;
static    void        contextswitch(Context *from, Context *to);


static void
taskstart(uint y, uint x)
{
    Task *t;
    ulong z;

    z = x<<16;    /* hide undefined 32-bit shift from 32-bit compilers */
    z <<= 16;
    z |= y;
    t = (Task*)z;

    t->startfn(t->startarg);
    taskexit(0);
}

static int taskidgen;

static Task*
taskalloc(void (*fn)(void*), void *arg, uint stack)
{
    Task *t;
    sigset_t zero;
    uint x, y;
    ulong z;

    /* allocate the task and stack together */
    t = calloc(sizeof *t+stack, 1);
    if(t == nil){
        fprint(2, "taskalloc malloc: %r\n");
        abort();
    }
    memset(t, 0, sizeof *t);
    t->stk = (uchar*)(t+1);
    t->stksize = stack;
    t->id = ++taskidgen;
    t->startfn = fn;
    t->startarg = arg;

    /* do a reasonable initialization */
    memset(&t->context.uc, 0, sizeof t->context.uc);
    sigemptyset(&zero);
    sigprocmask(SIG_BLOCK, &zero, &t->context.uc.uc_sigmask);

    /* must initialize with current context */
    if(getcontext(&t->context.uc) < 0){
        fprint(2, "getcontext: %r\n");
        abort();
    }

    /* call makecontext to do the real work. */
    /* leave a few words open on both ends */
    t->context.uc.uc_stack.ss_sp = t->stk+8;
    t->context.uc.uc_stack.ss_size = t->stksize-64;
#if defined(__sun__) && !defined(__MAKECONTEXT_V2_SOURCE)        /* sigh */
#warning "doing sun thing"
    /* can avoid this with __MAKECONTEXT_V2_SOURCE but only on SunOS 5.9 */
    t->context.uc.uc_stack.ss_sp = 
        (char*)t->context.uc.uc_stack.ss_sp
        +t->context.uc.uc_stack.ss_size;
#endif
    /*
     * All this magic is because you have to pass makecontext a
     * function that takes some number of word-sized variables,
     * and on 64-bit machines pointers are bigger than words.
     */
    z = (ulong)t;
    y = z;
    z >>= 16;    /* hide undefined 32-bit shift from 32-bit compilers */
    x = z>>16;
    makecontext(&t->context.uc, (void(*)())taskstart, 2, y, x);

    return t;
}

int
taskcreate(void (*fn)(void*), void *arg, uint stack)
{
    int id;
    Task *t;

    t = taskalloc(fn, arg, stack);
    taskcount++;
    id = t->id;
    if(nalltask % TASK_LIST_GROWTH == 0){
        alltask = realloc(alltask, (nalltask + TASK_LIST_GROWTH)*sizeof(alltask[0]));
        if(alltask == nil){
            fprint(2, "out of memory\n");
            abort();
        }
    }
    t->alltaskslot = nalltask;
    alltask[nalltask++] = t;
    taskready(t);
    return id;
}

void
tasksystem(void)
{
    if(!taskrunning->system){
        taskrunning->system = 1;
        --taskcount;
    }
}

void
taskswitch(void)
{
    needstack(0);
    contextswitch(&taskrunning->context, &taskschedcontext);
}

void
taskready(Task *t)
{
    t->ready = 1;
    addtask(&taskrunqueue, t);
}

int
taskyield(void)
{
    int n;
    
    n = tasknswitch;
    taskready(taskrunning);
    taskstate("yield");
    taskswitch();
    return tasknswitch - n - 1;
}

int
anyready(void)
{
    return taskrunqueue.head != nil;
}

void
taskexitall(int val)
{
    exit(val);
}

void
taskexit(int val)
{
    taskexitval = val;
    taskrunning->exiting = 1;
    taskswitch();
}

static void
contextswitch(Context *from, Context *to)
{
    if(swapcontext(&from->uc, &to->uc) < 0){
        fprint(2, "swapcontext failed: %r\n");
        assert(0);
    }
}

static void
taskscheduler(void)
{
    int i;
    Task *t;

    for(;;){
        if(taskcount == 0)
            exit(taskexitval);
        t = taskrunqueue.head;
        if(t == nil){
            fprint(2, "no runnable tasks! %d tasks stalled\n", taskcount);
            exit(1);
        }
        deltask(&taskrunqueue, t);
        t->ready = 0;
        taskrunning = t;
        tasknswitch++;
        contextswitch(&taskschedcontext, &t->context);
        taskrunning = nil;
        if(t->exiting){
            if(!t->system)
                taskcount--;
            i = t->alltaskslot;
            alltask[i] = alltask[--nalltask];
            alltask[i]->alltaskslot = i;
            free(t);
        }
    }
}

void**
taskdata(void)
{
    return &taskrunning->udata;
}

/*
 * debugging
 */
void
taskname(char *fmt, ...)
{
    va_list arg;
    Task *t;

    t = taskrunning;
    va_start(arg, fmt);
    vsnprint(t->name, sizeof t->name, fmt, arg);
    va_end(arg);
}

char*
taskgetname(void)
{
    return taskrunning->name;
}

void
taskstate(char *fmt, ...)
{
    va_list arg;
    Task *t;

    t = taskrunning;
    va_start(arg, fmt);
    vsnprint(t->state, sizeof t->name, fmt, arg);
    va_end(arg);
}

char*
taskgetstate(void)
{
    return taskrunning->state;
}

void
needstack(int n)
{
    Task *t;

    t = taskrunning;

    if((char*)&t <= (char*)t->stk
    || (char*)&t - (char*)t->stk < 256+n){
        fprint(2, "task stack overflow: &t=%p tstk=%p n=%d\n", &t, t->stk, 256+n);
        abort();
    }
}

bstring
taskgetinfo(void)
{
    int i;
    Task *t;

    bstring data;
    char* extra;
    bstring taskline;

    data = bfromcstr("{\"task_list\":[");

    for(i = 0; i < nalltask; i++)
    {
        t = alltask[i];

        if(t == taskrunning)
            extra = "running";
        else if(t->ready)
            extra = "ready";
        else
            extra = "";

        taskline = bformat("{\"id\": %d, \"system\": %d, \"name\": \"%s\", \"state\": \"%s\", \"extra\": \"%s\"}", t->id, t->system ? 1 : 0, t->name, t->state, extra);

        if(i < nalltask - 1)
        {
            bcatcstr(taskline, ",\n");
        }

        bconcat(data, taskline);
        bdestroy(taskline);
    }

    bcatcstr(data, "]}\n");

    return data;
}

/*
 * startup
 */

static int taskargc;
static char **taskargv;
int MAINSTACKSIZE = 32 * 1024;

static void
taskmainstart(void *v)
{
    taskname("taskmain");
    taskmain(taskargc, taskargv);
}

int
main(int argc, char **argv)
{
    argv0 = argv[0];
    taskargc = argc;
    taskargv = argv;

    taskcreate(taskmainstart, nil, MAINSTACKSIZE);
    taskscheduler();

    fprint(2, "taskscheduler returned in main!\n");
    abort();
    return 0;
}

/*
 * hooray for linked lists
 */
void
addtask(Tasklist *l, Task *t)
{
    if(l->tail){
        l->tail->next = t;
        t->prev = l->tail;
    }else{
        l->head = t;
        t->prev = nil;
    }
    l->tail = t;
    t->next = nil;
}

void
deltask(Tasklist *l, Task *t)
{
    if(t->prev)
        t->prev->next = t->next;
    else
        l->head = t->next;
    if(t->next)
        t->next->prev = t->prev;
    else
        l->tail = t->prev;
}

unsigned int
taskid(void)
{
    return taskrunning->id;
}


Task *taskself()
{
    return taskrunning;
}

unsigned int  taskgetid(Task *task)
{
    return task->id;
}
