#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>

#ifdef LEVEL3
#include <stdatomic.h>
#endif

typedef struct {
    unsigned int frame_num;
    struct timespec timestamp;
    char mock_data[10];
} camera_frame_t;

typedef struct {
    double x, y, z; //pozycja
    double roll, pitch, yaw; //orientacja 
    struct timespec timestamp;
} robot_state_t;

#if defined(LEVEL2) || defined(LEVEL3)

#endif

#endif


