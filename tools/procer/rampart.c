#include "procer.h"


int Rampart_wait(Rampart *on[])
{
    int i = 0;

    for(i = 0; on[i] != NULL; i++) {
        if(Rampart_should_wait(on[i])) {
            tasksleep(&(on[i]->wait));
        }
    }

    return 1;
}

void Rampart_running(Rampart *on)
{
    on->running = 1; 
    on->failed = 0;
    taskwakeupall(&on->wait);
}

void Rampart_notrunning(Rampart *on)
{
    on->running = 0;
}

void Rampart_failed(Rampart *on)
{
    on->running = 0;
    on->failed = 1;
    taskwakeupall(&on->wait);
}


