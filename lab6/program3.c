#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// --- STRUKTURY DANYCH (Zadanie 2) ---
typedef struct {
  unsigned int frame_num;
  struct timespec timestamp;
} camera_frame_t;

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

// --- ZMIENNE GLOBALNE I ATOMOWE (Zadanie 3) ---
volatile sig_atomic_t keep_running = 1;

frame_buffer_t left_buf, right_buf, sync_buf;

// Liczniki atomowe - bezpieczne bez mutexów
atomic_long stat_left_frames = 0;
atomic_long stat_right_frames = 0;
atomic_long stat_sync_pairs = 0;
atomic_long stat_robot_data = 0;

// --- FUNKCJE POMOCNICZE ---
void buffer_init(frame_buffer_t *buf, int capacity) {
  buf->buffer = malloc(sizeof(camera_frame_t) * capacity);
  buf->head = buf->tail = buf->count = 0;
  buf->capacity = capacity;
  pthread_mutex_init(&buf->mutex, NULL);
  pthread_cond_init(&buf->cond_not_empty, NULL);
  pthread_cond_init(&buf->cond_not_full, NULL);
}

void buffer_push(frame_buffer_t *buf, camera_frame_t frame) {
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == buf->capacity && keep_running)
    pthread_cond_wait(&buf->cond_not_full, &buf->mutex);
  if (!keep_running) {
    pthread_mutex_unlock(&buf->mutex);
    return;
  }
  buf->buffer[buf->tail] = frame;
  buf->tail = (buf->tail + 1) % buf->capacity;
  buf->count++;
  pthread_cond_signal(&buf->cond_not_empty);
  pthread_mutex_unlock(&buf->mutex);
}

bool buffer_pop(frame_buffer_t *buf, camera_frame_t *frame) {
  pthread_mutex_lock(&buf->mutex);
  while (buf->count == 0 && keep_running)
    pthread_cond_wait(&buf->cond_not_empty, &buf->mutex);
  if (!keep_running && buf->count == 0) {
    pthread_mutex_unlock(&buf->mutex);
    return false;
  }
  *frame = buf->buffer[buf->head];
  buf->head = (buf->head + 1) % buf->capacity;
  buf->count--;
  pthread_cond_signal(&buf->cond_not_full);
  pthread_mutex_unlock(&buf->mutex);
  return true;
}

// --- WĄTEK WATCHDOG (Zadanie 3) ---
void *watchdog_thread(void *arg) {
  long prev_left = 0, prev_robot = 0;

  while (keep_running) {
    sleep(1); // Sprawdzaj co sekundę

    long curr_left = atomic_load(&stat_left_frames);
    long curr_robot = atomic_load(&stat_robot_data);

    // Jeśli przez sekundę przybyło mniej niż 20 klatek (powinno być 25)
    if (curr_left - prev_left < 20) {
      // POPRAWNA LINIA:
      printf("\033[1;31m[WATCHDOG] OSTRZEŻENIE: ROBOT SLOW / DEADLINE "
             "EXCEEDED! (%ld)\033[0m\n",
             curr_robot - prev_robot);
    }

    // Jeśli przez sekundę przybyło mniej niż 90 danych robota (powinno być 100)
    if (curr_robot - prev_robot < 90) {
      printf("\033[1;31m[WATCHDOG] OSTRZEŻENIE: ROBOT SLOW / DEADLINE EXCEEDED! (Aktualnie: %ld)\033[0m\n",
             curr_robot - prev_robot);
    }

    prev_left = curr_left;
    prev_robot = curr_robot;
  }
  return NULL;
}

// --- POZOSTAŁE WĄTKI ---
void *camera_left_thread(void *arg) {
  unsigned int counter = 0;
  while (keep_running) {
    camera_frame_t f = {++counter, {0}};
    clock_gettime(CLOCK_REALTIME, &f.timestamp);
    buffer_push(&left_buf, f);
    atomic_fetch_add(&stat_left_frames, 1);
    usleep(40000); // 25 Hz
  }
  return NULL;
}

void *camera_right_thread(void *arg) {
  unsigned int counter = 0;
  while (keep_running) {
    camera_frame_t f = {++counter, {0}};
    clock_gettime(CLOCK_REALTIME, &f.timestamp);
    buffer_push(&right_buf, f);
    atomic_fetch_add(&stat_right_frames, 1);
    usleep(40000); // 25 Hz
  }
  return NULL;
}

void *robot_thread(void *arg) {
  while (keep_running) {
    atomic_fetch_add(&stat_robot_data, 1);
    usleep(10000); // 100 Hz
  }
  return NULL;
}

// --- FUNKCJA USTAWIAJĄCA PRIORYTET CZASU RZECZYWISTEGO ---
void set_realtime_priority(pthread_t thread, int priority) {
  struct sched_param param;
  param.sched_priority = priority;
  if (pthread_setschedparam(thread, SCHED_FIFO, &param) != 0) {
    perror("[SYSTEM] Nie udało się ustawić priorytetu (użyj sudo?)");
  }
}

void handle_sigint(int sig) { keep_running = 0; }

int main() {
  signal(SIGINT, handle_sigint);
  buffer_init(&left_buf, 10);
  buffer_init(&right_buf, 10);

  pthread_t t_l, t_r, t_robot, t_watch;

  // Tworzenie wątków
  pthread_create(&t_l, NULL, camera_left_thread, NULL);
  pthread_create(&t_r, NULL, camera_right_thread, NULL);
  pthread_create(&t_robot, NULL, robot_thread, NULL);
  pthread_create(&t_watch, NULL, watchdog_thread, NULL);

  // USTAWIANIE PRIORYTETÓW (Zadanie 3)
  set_realtime_priority(t_robot, 50); // Robot najważniejszy
  set_realtime_priority(t_l, 40);     // Kamery wysoki priorytet
  set_realtime_priority(t_r, 40);

  printf("System CZASU RZECZYWISTEGO działa. CTRL+C aby zakończyć.\n");

  // Czekanie na koniec
  pthread_join(t_watch, NULL);

  // Budzenie wątków do zamknięcia
  pthread_cond_broadcast(&left_buf.cond_not_empty);
  pthread_cond_broadcast(&right_buf.cond_not_empty);
  pthread_join(t_l, NULL);
  pthread_join(t_r, NULL);
  pthread_join(t_robot, NULL);

  // GENEROWANIE RAPORTU (Zadanie 3)
  FILE *f = fopen("report.txt", "w");
  if (f) {
    fprintf(f, "=== RAPORT KOŃCOWY SYSTEMU ROBOTA ===\n");
    fprintf(f, "Liczba klatek Lewa Kamera : %ld\n",
            atomic_load(&stat_left_frames));
    fprintf(f, "Liczba klatek Prawa Kamera: %ld\n",
            atomic_load(&stat_right_frames));
    fprintf(f, "Liczba danych Robota      : %ld\n",
            atomic_load(&stat_robot_data));
    fclose(f);
    printf("\n[SYSTEM] Raport zapisany w report.txt\n");
  }

  return 0;
}