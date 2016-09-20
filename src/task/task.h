/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#ifndef _TASK_H_
#define _TASK_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>
#include "zmq_compat.h"

struct tns_value_t;

/*
 * basic procs and threads
 */

typedef struct Task Task;
typedef struct Tasklist Tasklist;

#define MAX_STATE_LENGTH 30

int    anyready(void);
int    taskcreate(void (*f)(void *arg), void *arg, unsigned int stacksize);
void    taskexit(int);
void    taskexitall(int);
void    taskmain(int argc, char *argv[]);
int    taskyield(void);
void**    taskdata(void);
void    needstack(int);
void    taskname(char*);
void    taskstate(char*);
char*    taskgetname(void);
char*    taskgetstate(void);
struct tns_value_t *taskgetinfo(void);
void    tasksystem(void);
unsigned int  taskdelay(unsigned int);
unsigned int  taskid(void);
unsigned int  taskgetid(Task *task);
int taskwaiting();
int tasksrunning();
void taskready(Task *t);
Task *taskself();
void taskswitch();

struct Tasklist  /* used internally */
{
  Task  *head;
  Task  *tail;
};

/** Signal management. **/

int tasksignal(Task *task, int signal);
int task_was_signaled();
void task_clear_signal();
int taskallsignal(int signal);

/*
 * queuing locks
 */
typedef struct QLock QLock;
struct QLock
{
  Task  *owner;
  Tasklist waiting;
};

void  qlock(QLock*);
int  canqlock(QLock*);
void  qunlock(QLock*);

/*
 * reader-writer locks
 */
typedef struct RWLock RWLock;
struct RWLock
{
  int  readers;
  Task  *writer;
  Tasklist rwaiting;
  Tasklist wwaiting;
};

void  rlock(RWLock*);
int  canrlock(RWLock*);
void  runlock(RWLock*);

void  wlock(RWLock*);
int  canwlock(RWLock*);
void  wunlock(RWLock*);

/*
 * sleep and wakeup (condition variables)
 */
typedef struct Rendez Rendez;

struct Rendez
{
  QLock  *l;
  Tasklist waiting;
};

void  tasksleep(Rendez*);
int  taskwakeup(Rendez*);
int  taskwakeupall(Rendez*);
int taskbarrier(Rendez *r);



/*
 * Threaded I/O.
 */
int fdread(int, void*, int);
int fdread1(int, void*, int);  /* always uses fdwait */
int fdrecv1(int, void*, int);  /* always uses fdwait */
int fdwrite(int, void*, int);
int fdsend(int, void*, int);
int fdrecv(int, void*, int);
int fdwait(int, int);
int fdnoblock(int);
void fdshutdown();

extern Task *FDTASK;

#define fdclose(fd) if(fd >= 0) close(fd)

void    fdtask(void*);

/*
 * 0mq Integration.
 */
void mqinit(int threads);
void *mqsocket(int type);
int mqwait(void *socket, int rw);
int mqrecv(void *socket, zmq_msg_t *msg, int flags);
int mqsend(void *socket, zmq_msg_t *msg, int flags);

extern void *ZMQ_CTX;

/*
 * Network dialing - sets non-blocking automatically
 */
enum
{
  UDP = 0,
  TCP = 1,
};

int    netannounce(int, char*, int);
int    netaccept(int, char*, int*, int);
int    netdial(int, char*, int);
int    netlookup(char*, uint32_t*);  /* blocks entire program! */

#ifdef __cplusplus
}
#endif
#endif
