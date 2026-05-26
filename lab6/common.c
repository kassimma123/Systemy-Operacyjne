#include "common.h"
#include <stdlib.h>

double timespec_diff_ms(struct timespec *start, struct timespec *end){
    double start_ms = (start->tv_sec * 1000.0) + (start->tv_nsec / 1000000.0);
    double end_ms = (end->tv_sec * 1000.0) + (end->tv_nsec / 1000000.0);
    return end_ms - start_ms;
}

void add_ms_to_timespec(struct timespec *ts, long ms){
    ts->tv_nsec += (ms % 1000) * 1000000L;
    ts->tv_sec += ms / 1000;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec += 1;
    }
}

#if defined(LEVEL2) || defined(LEVEL3)
#endif