#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// struktury
typedef struct {
  unsigned int frame_num;
  struct timespec timestamp;
} camera_frame_t;

typedef struct {
  double x, y, z;
  struct timespec timestamp;
} robot_state_t;

// bufor
typedef struct {
  camera_frame_t *buffer;
  int head;     // skad
  int tail;     // gdzie
  int count;    // ile
  int capacity; // max

  pthread_mutex_t mutex;
  pthread_cond_t cond_not_empty; // sa dane
  pthread_cond_t cond_not_full; // jest miejsce
} frame_buffer_t;

// zmienne
volatile sig_atomic_t keep_running = 1;

frame_buffer_t left_buf;
frame_buffer_t right_buf;
frame_buffer_t sync_buf;

robot_state_t global_robot_state;
pthread_mutex_t robot_mutex = PTHREAD_MUTEX_INITIALIZER;

// staty
long stat_frames_generated = 0;
long stat_sync_pairs = 0;
long stat_robot_data = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// obsluga sygnalu ctrl+c
void handle_sigint(int sig) {
  printf("\n[SYSTEM] Otrzymano sygnał CTRL+C. Trwa bezpieczne zamykanie...\n");
  keep_running = 0;
}

// czas
double diff_ms(struct timespec *start, struct timespec *end) {
  double start_ms = (start->tv_sec * 1000.0) + (start->tv_nsec / 1000000.0);
  double end_ms = (end->tv_sec * 1000.0) + (end->tv_nsec / 1000000.0);
  return end_ms - start_ms;
}

void sleep_ms(int ms) { usleep(ms * 1000); }

// bufor
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

void buffer_destroy(frame_buffer_t *buf) {
  free(buf->buffer);
  pthread_mutex_destroy(&buf->mutex);
  pthread_cond_destroy(&buf->cond_not_empty);
  pthread_cond_destroy(&buf->cond_not_full);
}

// do bufora
bool buffer_push(frame_buffer_t *buf, camera_frame_t frame) {
  pthread_mutex_lock(&buf->mutex);

  // zmienne warunkowe zawsze sprawdzamy w petli while!
  while (buf->count == buf->capacity && keep_running) {
    pthread_cond_wait(&buf->cond_not_full, &buf->mutex);
  }

  if (!keep_running) { // jesli zamykamy program, przerywamy
    pthread_mutex_unlock(&buf->mutex);
    return false;
  }

  buf->buffer[buf->tail] = frame;
  buf->tail = (buf->tail + 1) % buf->capacity;
  buf->count++;

  pthread_cond_signal(
      &buf->cond_not_empty); // budzi watek, ktory czeka na pobranie
  pthread_mutex_unlock(&buf->mutex);
  return true;
}

// z bufora
bool buffer_pop(frame_buffer_t *buf, camera_frame_t *frame) {
  pthread_mutex_lock(&buf->mutex);

  while (buf->count == 0 && keep_running) {
    pthread_cond_wait(&buf->cond_not_empty, &buf->mutex);
  }

  if (!keep_running && buf->count == 0) {
    pthread_mutex_unlock(&buf->mutex);
    return false;
  }

  *frame = buf->buffer[buf->head];
  buf->head = (buf->head + 1) % buf->capacity;
  buf->count--;

  pthread_cond_signal(
      &buf->cond_not_full); // budzi watek, ktory chce wstawic, ale mial pelno
  pthread_mutex_unlock(&buf->mutex);
  return true;
}

// watki kamer
void *camera_thread(void *arg) {
  frame_buffer_t *my_buf = (frame_buffer_t *)arg;
  unsigned int counter = 0;

  while (keep_running) {
    camera_frame_t frame;
    frame.frame_num = ++counter;
    clock_gettime(CLOCK_REALTIME, &frame.timestamp);

    if (!buffer_push(my_buf, frame))
      break;

    pthread_mutex_lock(&stats_mutex);
    stat_frames_generated++;
    pthread_mutex_unlock(&stats_mutex);

    sleep_ms(40);
  }
  return NULL;
}

// watek synchronizacji
void *sync_thread(void *arg) {
  camera_frame_t left, right;

  while (keep_running) {
    // pobierz klatki z obu buforow
    if (!buffer_pop(&left_buf, &left))
      break;
    if (!buffer_pop(&right_buf, &right))
      break;

    double diff = diff_ms(&left.timestamp, &right.timestamp);
    if (diff < 0)
      diff = -diff;

    if (diff < 20.0) { // tol 20ms
      // tylko lewa
      // zapisu
      if (!buffer_push(&sync_buf, left))
        break;

      pthread_mutex_lock(&stats_mutex);
      stat_sync_pairs++;
      pthread_mutex_unlock(&stats_mutex);
    }
  }
  return NULL;
}

// watek zapisu obrazow
void *save_thread(void *arg) {
  camera_frame_t sync_frame;
  while (keep_running) {
    if (buffer_pop(&sync_buf, &sync_frame)) {
      // zapis na dysk
      sleep_ms(100);
    }
  }
  return NULL;
}

// watek stanu robota
void *robot_thread(void *arg) {
  double pos = 0.0;
  while (keep_running) {
    pthread_mutex_lock(&robot_mutex);
    global_robot_state.x = pos;
    global_robot_state.y = pos * 1.5;
    clock_gettime(CLOCK_REALTIME, &global_robot_state.timestamp);
    pthread_mutex_unlock(&robot_mutex);

    pthread_mutex_lock(&stats_mutex);
    stat_robot_data++;
    pthread_mutex_unlock(&stats_mutex);

    pos += 0.1;
    sleep_ms(10);
  }
  return NULL;
}

// watek statystyk
void *stats_thread(void *arg) {
  while (keep_running) {
    sleep_ms(2000); // wyswietlaj co 2 sekundy
    if (!keep_running)
      break;

    pthread_mutex_lock(&stats_mutex);
    printf("--- STATYSTYKI ---\n");
    printf("Wygenerowane klatki : %ld\n", stat_frames_generated);
    printf("Pary stereo (sync)  : %ld\n", stat_sync_pairs);
    printf("Dane stanu robota   : %ld\n", stat_robot_data);
    printf("------------------\n");
    pthread_mutex_unlock(&stats_mutex);
  }
  return NULL;
}

// funkcja glowna
int main() {
  // podpiecie sygnalu ctrl+c
  signal(SIGINT, handle_sigint);

  printf("Uruchamianie systemu poziomu 2...\n");
  printf("Nacisnij CTRL+C aby bezpiecznie zamknac program.\n\n");

  // inicjalizacja buforow (pojemnosc np. 10 elementow)
  buffer_init(&left_buf, 10);
  buffer_init(&right_buf, 10);
  buffer_init(&sync_buf, 10);

  pthread_t t_cam_l, t_cam_r, t_sync, t_save, t_robot, t_stats;

  // przekazujemy adresy buforow jako argumenty do watkow kamer
  pthread_create(&t_cam_l, NULL, camera_thread, &left_buf);
  pthread_create(&t_cam_r, NULL, camera_thread, &right_buf);

  pthread_create(&t_sync, NULL, sync_thread, NULL);
  pthread_create(&t_save, NULL, save_thread, NULL);
  pthread_create(&t_robot, NULL, robot_thread, NULL);
  pthread_create(&t_stats, NULL, stats_thread, NULL);

  // czekamy na zakonczenie watkow (nastapi to po nacisnieciu ctrl+c)
  // jednak musimy wybudzic watki, ktore utknely czekajac na dane!
  pthread_join(t_stats, NULL); // ten watek na pewno sie zamknie

  // budzenie wszystkich "spiacych" watkow w buforach
  pthread_cond_broadcast(&left_buf.cond_not_empty);
  pthread_cond_broadcast(&left_buf.cond_not_full);
  pthread_cond_broadcast(&right_buf.cond_not_empty);
  pthread_cond_broadcast(&right_buf.cond_not_full);
  pthread_cond_broadcast(&sync_buf.cond_not_empty);
  pthread_cond_broadcast(&sync_buf.cond_not_full);

  pthread_join(t_cam_l, NULL);
  pthread_join(t_cam_r, NULL);
  pthread_join(t_sync, NULL);
  pthread_join(t_save, NULL);
  pthread_join(t_robot, NULL);

  // sprzatanie pamieci
  buffer_destroy(&left_buf);
  buffer_destroy(&right_buf);
  buffer_destroy(&sync_buf);

  printf("System poprawnie zamkniety. Do widzenia!\n");
  return 0;
}