// common.c
#include "common.h"
#include <stdlib.h>

double timespec_diff_ms(struct timespec *start, struct timespec *end) {
  double start_ms = (start->tv_sec * 1000.0) + (start->tv_nsec / 1000000.0);
  double end_ms = (end->tv_sec * 1000.0) + (end->tv_nsec / 1000000.0);
  return end_ms - start_ms;
}

void add_ms_to_timespec(struct timespec *ts, long ms) {
  ts->tv_nsec += (ms % 1000) * 1000000L;
  ts->tv_sec += ms / 1000;
  if (ts->tv_nsec >= 1000000000L) {
    ts->tv_nsec -= 1000000000L;
    ts->tv_sec += 1;
  }
}

#if defined(LEVEL2) || defined(LEVEL3)
void buffer_init(frame_buffer_t *buf, int capacity) {
  buf->buffer = malloc(sizeof(camera_frame_t) * capacity);
  buf->head = 0;
  buf->tail = 0;
  buf->count = 0;
  buf->capacity = capacity;
  pthread_mutex_init(&buf->mutex, NULL);
  pthread_cond_init(&buf->cond_not_empty, NULL);
  pthread_cond_init(&buf->cond_not_full, NULL);
}

void buffer_push(frame_buffer_t *buf, camera_frame_t frame) {
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == buf->capacity) {
    pthread_cond_wait(&buf->cond_not_full, &buf->mutex);
  }
  buf->buffer[buf->head] = frame;
  buf->head = (buf->head + 1) % buf->capacity;
  buf->count++;
  pthread_cond_signal(&buf->cond_not_empty);
  pthread_mutex_unlock(&buf->mutex);
}

camera_frame_t buffer_pop(frame_buffer_t *buf) {
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == 0) {
    pthread_cond_wait(&buf->cond_not_empty, &buf->mutex);
  }
  camera_frame_t frame = buf->buffer[buf->tail];
  buf->tail = (buf->tail + 1) % buf->capacity;
  buf->count--;
  pthread_cond_signal(&buf->cond_not_full);
  pthread_mutex_unlock(&buf->mutex);
  return frame;
}
#endif