// common.h
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#ifdef LEVEL3
#include <stdatomic.h>
#endif

// ramka
typedef struct {
  unsigned int frame_num;
  struct timespec timestamp;
  char mock_data[10]; // symulacja dane obrazu
} camera_frame_t;

// stan robota
typedef struct {
  double x, y, z;          // pozycja
  double roll, pitch, yaw; // orientacja
  struct timespec timestamp;
} robot_state_t;

#if defined(LEVEL2) || defined(LEVEL3)
// bufor cykliczny
typedef struct {
  camera_frame_t *buffer;
  int head;
  int tail;
  int count;
  int capacity;
  pthread_mutex_t mutex;
  pthread_cond_t cond_not_empty;
  pthread_cond_t cond_not_full;
} frame_buffer_t;
#endif

// pomocnicze
double timespec_diff_ms(struct timespec *start, struct timespec *end);
void add_ms_to_timespec(struct timespec *ts, long ms);

#endif