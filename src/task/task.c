/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include "taskimpl.h"
#include <fcntl.h>
#include <stdio.h>
#include "dbg.h"
#include "tnetstrings.h"
#include "tnetstrings_impl.h"

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

static    void        contextswitch(Context *from, Context *to);

static void taskstart(uint y, uint x)
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

static Task* taskalloc(void (*fn)(void*), void *arg, uint stack)
{
    Task *t = NULL;
    sigset_t zero;
    uint x = 0;
    uint y = 0;
    ulong z = 0L;

    /* allocate the task and stack together */
    t = calloc(sizeof(Task) + stack, 1);
    check_mem(t);

    t->stk = (uchar*)(t+1);
    t->stksize = stack;
    t->id = ++taskidgen;
    t->startfn = fn;
    t->startarg = arg;

    /* do a reasonable initialization */
    sigemptyset(&zero);
    sigprocmask(SIG_BLOCK, &zero, &t->context.uc.uc_sigmask);

    /* must initialize with current context */
    check(getcontext(&t->context.uc) >= 0, "getcontext failed.");

    /* call makecontext to do the real work. */
    /* leave a few words open on both ends */
    t->context.uc.uc_stack.ss_sp = (char *)t->stk+8;
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

error:
    abort();
}

int taskcreate(void (*fn)(void*), void *arg, uint stack)
{
    Task *t  = taskalloc(fn, arg, stack);
    int id = t->id;

    taskcount++;

    if(nalltask % TASK_LIST_GROWTH == 0){
        alltask = realloc(alltask, (nalltask + TASK_LIST_GROWTH)*sizeof(alltask[0]));
        check_mem(alltask);
    }

    t->alltaskslot = nalltask;
    alltask[nalltask++] = t;
    taskready(t);

    return id;

error:
    return -1;
}

void tasksystem(void)
{
    if(!taskrunning->system) {
        taskrunning->system = 1;
        --taskcount;
    }
}

void taskswitch(void)
{
    needstack(0);
    contextswitch(&taskrunning->context, &taskschedcontext);
}

void taskready(Task *t)
{
    t->ready = 1;
    addtask(&taskrunqueue, t);
}

int taskyield(void)
{
    int n = tasknswitch;
    taskready(taskrunning);
    taskstate("yield");
    taskswitch();

    return tasknswitch - n - 1;
}

int anyready(void)
{
    return taskrunqueue.head != NULL;
}

void taskexitall(int val)
{
    exit(val);
}

void taskexit(int val)
{
    taskexitval = val;
    taskrunning->exiting = 1;
    taskswitch();
}

static void contextswitch(Context *from, Context *to)
{
    if(swapcontext(&from->uc, &to->uc) < 0){
        log_err("swapcontext failed.");
        abort();
    }
}

int tasksrunning()
{
    int i = 0;

    debug("Tasks running nalltask=%d, taskcount=%d", nalltask, taskcount);
    for(i = 0; i < nalltask; i++) {
        debug("RUNNING id=%d:%p", alltask[i]->id, alltask[i]);
    }

    return nalltask;
}


static void taskscheduler(void)
{
    int i = 0;
    Task *t = NULL;

    for(;;){
        if(taskcount == 0) {
            exit(taskexitval);
        }

        t = taskrunqueue.head;

        if(t == NULL) {
            debug("Nothing runnable, here's the tasks that are stalled (%d,%d):", nalltask, taskcount);
            for(i = 0; i < nalltask; i++) {
                debug("STALLED id=%d:%p", alltask[i]->id, alltask[i]);
            }
        }

        check(t != NULL, "No runnable tasks, %d tasks stalled", taskcount);

        deltask(&taskrunqueue, t);

        t->ready = 0;
        taskrunning = t;
        tasknswitch++;

        contextswitch(&taskschedcontext, &t->context);
        taskrunning = NULL;

        if(t->exiting){
            if(!t->system) {
                taskcount--;
            }

            i = t->alltaskslot;
            alltask[i] = alltask[--nalltask];
            alltask[i]->alltaskslot = i;
            debug("FREEING TASK: %p", t);
            free(t);
        }
    }

error:
    abort();
}

/*
 * debugging
 */
void taskname(char *name)
{
    int len = strlen(name);
    memcpy(taskrunning->name, name, len < MAX_STATE_LENGTH ? len : MAX_STATE_LENGTH);
    taskrunning->name[len] = '\0';
}

char* taskgetname(void)
{
    return taskrunning->name;
}


void taskstate(char *state)
{
    int len = strlen(state);
    memcpy(taskrunning->state, state, len < MAX_STATE_LENGTH ? len : MAX_STATE_LENGTH);
    taskrunning->state[len] = '\0';
}

char* taskgetstate(void)
{
    return taskrunning->state;
}

void needstack(int n)
{
    Task *t = taskrunning;

    if((char*)&t <= (char*)t->stk
    || (char*)&t - (char*)t->stk < 256+n) {
        fprintf(stderr, "task stack overflow: &t=%p tstk=%p n=%d\n", &t, t->stk, 256+n);
    }
}

struct tagbstring TASKINFO_HEADERS = bsStatic("38:2:id,6:system,4:name,5:state,6:status,]");

tns_value_t *taskgetinfo(void)
{
    int i = 0;
    Task *t = NULL;
    tns_value_t *rows = tns_new_list();
    char* status = NULL;

    for(i = 0; i < nalltask; i++)
    {
        t = alltask[i];

        if(t == taskrunning) {
            status = "running";
        } else if(t->ready) {
            status = "ready";
        } else if(t->exiting) {
            status = "exiting";
        } else {
            status = "idle";
        }

        tns_value_t *el = tns_new_list();
        tns_add_to_list(el, tns_new_integer(t->id));
        tns_add_to_list(el, t->system ? tns_get_true() : tns_get_false());
        tns_add_to_list(el, tns_parse_string(t->name, strlen(t->name)));
        tns_add_to_list(el, tns_parse_string(t->state, strlen(t->state)));
        tns_add_to_list(el, tns_parse_string(status, strlen(status)));

        tns_add_to_list(rows, el);
    }

    return tns_standard_table(&TASKINFO_HEADERS, rows);
}

/*
 * startup
 */

static int taskargc;
static char **taskargv;
int MAINSTACKSIZE = 96 * 1024;

static void taskmainstart(void *v)
{
    (void)v;
    taskname("taskmain");
    taskmain(taskargc, taskargv);
}

int main(int argc, char **argv)
{
    taskargc = argc;
    taskargv = argv;

    taskcreate(taskmainstart, NULL, MAINSTACKSIZE);
    taskscheduler();

    fprintf(stderr, "taskscheduler returned in main!\n");
    abort();

    return 0;
}

/*
 * hooray for linked lists
 */
void addtask(Tasklist *l, Task *t)
{
    if(t->prev != NULL || t->next != NULL || l->head == t) {
        log_err("Task is already added, refusing to do it again: %p, %p, %p=%p", t->prev, t->next, l->head, t);
        return;
    }

#ifndef NDEBUG
    Task *test = NULL;

    for(test = taskrunqueue.head; test != NULL; test = test->next)
    {
        assert(test != t && "Fucking double addtask mother fucker!");
    }
#endif

    if(l->tail) {
        l->tail->next = t;
        t->prev = l->tail;
    } else {
        l->head = t;
        t->prev = NULL;
    }

    l->tail = t;
    t->next = NULL;
}

void deltask(Tasklist *l, Task *t)
{
    if(t->prev) {
        t->prev->next = t->next;
    } else {
        l->head = t->next;
    }

    if(t->next) {
        t->next->prev = t->prev;
    } else {
        l->tail = t->prev;
    }

    t->next = NULL;
    t->prev = NULL;
}

unsigned int taskid(void)
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

int tasksignal(Task *task, int signal)
{
    check(task != NULL, "Task was NULL, that's really bad.");
    check(signal > 0, "Signal has to be greater than 0.");

    if(task->signal == 0) {
        task->signal = signal;
        taskready(task);
    } else {
        log_info("Task %p is already signaled with %d", task, task->signal);
    }

    return 0;
error:
    return -1;
}

int taskallsignal(int signal)
{
    debug("Sending %d to all tasks.", signal);
    int i = 0;
    Task *t = NULL;
    check(signal > 0, "Signal must be greater than 0.");

    if(FDTASK != NULL) {
        debug("MAKE THE FDTASK SIGNAL FIRST");
        FDTASK->signal = signal;
        taskdelay(1);
        debug("BACK NOW CLEAR THE REST.");
    }

    // first signal everything that's already running
    for(t = taskrunqueue.head; t != NULL ; t = t->next) {
        if(t != FDTASK && !t->exiting && t->signal == 0) {
            debug("SIGNAL RUNNING: id=%d, %p", t->id, t);
            t->signal = signal;
        }
    }

    // then signal the tasks that are idle
    for(i = 0; i < nalltask; i++)
    {
        t = alltask[i];
        // don't signal any exiting tasks, ourself, or ones that are already signaled
        if(t != NULL && !t->exiting && t != taskrunning && t->signal == 0 && t->next == NULL && t->prev == NULL) {
            debug("SIGNAL IDLE: id=%d, %p", t->id, t);
            t->signal = signal;
            taskready(t);
        }
    }

    while((i = taskyield()) > 0) {
        // debug("BACK FROM THE FINAL SWITCH! TASK YIELD: %d", i);
    }

    return 0;
error:
    return -1;
}

int task_was_signaled()
{
    return taskrunning->signal;
}

void task_clear_signal()
{
    taskrunning->signal = 0;
}
