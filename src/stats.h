#ifndef _stats_h
#define _stats_h

typedef struct Statistics {
    long long int event_counter;
} Statistics;

extern Statistics collector_stats;

#endif
