/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

#if defined(__sun__)
#    define __EXTENSIONS__ 1 /* SunOS */
#    if defined(__SunOS5_6__) || defined(__SunOS5_7__) || defined(__SunOS5_8__)
        /* NOT USING #define __MAKECONTEXT_V2_SOURCE 1 / * SunOS */
#    else
#        define __MAKECONTEXT_V2_SOURCE 1
#    endif
#endif

#define USE_UCONTEXT 1

#if defined(__OpenBSD__)
#undef USE_UCONTEXT
#define USE_UCONTEXT 0
#endif

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_5)
#undef USE_UCONTEXT
#define USE_UCONTEXT 0
#endif
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#if USE_UCONTEXT
#include <ucontext.h>
#endif
#include <sys/utsname.h>
#include <inttypes.h>
#include "task.h"

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#define ulong task_ulong
#define uint task_uint
#define uchar task_uchar
#define ushort task_ushort
#define uvlong task_uvlong
#define vlong task_vlong

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long long uvlong;
typedef long long vlong;

#if defined(__FreeBSD__) && __FreeBSD__ < 5
extern    int        getmcontext(mcontext_t*);
extern    void        setmcontext(const mcontext_t*);
#define    setcontext(u)    setmcontext(&(u)->uc_mcontext)
#define    getcontext(u)    getmcontext(&(u)->uc_mcontext)
extern    int        swapcontext(ucontext_t*, const ucontext_t*);
extern    void        makecontext(ucontext_t*, void(*)(), int, ...);
#endif

#if defined(__APPLE__)
#    define mcontext libthread_mcontext
#    define mcontext_t libthread_mcontext_t
#    define ucontext libthread_ucontext
#    define ucontext_t libthread_ucontext_t
#    if defined(__i386__)
#        include "386-ucontext.h"
#    elif defined(__x86_64__)
#        include "amd64-ucontext.h"
#    else
#        include "power-ucontext.h"
#    endif    
#endif

#if defined(__OpenBSD__)
#    define mcontext libthread_mcontext
#    define mcontext_t libthread_mcontext_t
#    define ucontext libthread_ucontext
#    define ucontext_t libthread_ucontext_t
#    if defined __i386__
#        include "386-ucontext.h"
#    elif defined(__x86_64__)
#        include "amd64-ucontext.h"
#    else
#        include "power-ucontext.h"
#    endif
extern pid_t rfork_thread(int, void*, int(*)(void*), void*);
#endif

#if 0 &&  defined(__sun__)
#    define mcontext libthread_mcontext
#    define mcontext_t libthread_mcontext_t
#    define ucontext libthread_ucontext
#    define ucontext_t libthread_ucontext_t
#    include "sparc-ucontext.h"
#endif

typedef struct Context Context;

enum
{
    STACK = 8192
};

struct Context
{
    ucontext_t    uc;
};

struct Task
{
    char    name[MAX_STATE_LENGTH];    // offset known to acid
    char    state[MAX_STATE_LENGTH];
    Task    *next;
    Task    *prev;
    Context    context;
    uvlong    alarmtime;
    uint    id;
    uchar    *stk;
    uint    stksize;
    int    exiting;
    int    alltaskslot;
    int    system;
    int    ready;
    void    (*startfn)(void*);
    void    *startarg;
    int signal;
};

void    taskready(Task*);
void    taskswitch(void);

void    addtask(Tasklist*, Task*);
void    deltask(Tasklist*, Task*);

extern Task    *taskrunning;
extern int    taskcount;
