#ifndef _state_h
#define _state_h

typedef struct State {
    int cs;
} State;

int State_exec(State *state, int event);
int State_finish(State *state);
int State_init(State *state);

#endif
