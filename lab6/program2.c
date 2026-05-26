#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// --- STRUKTURY DANYCH ---
typedef struct {
  unsigned int frame_num;
  struct timespec timestamp;
} camera_frame_t;

typedef struct {
  double x, y, z;
  struct timespec timestamp;
} robot_state_t;

// --- STRUKTURA BUFORA CYKLICZNEGO (FIFO) ---
typedef struct {
  camera_frame_t *buffer;
  int head;     // Skąd pobieramy
  int tail;     // Gdzie wstawiamy
  int count;    // Aktualna liczba elementów
  int capacity; // Maksymalna pojemność

  pthread_mutex_t mutex;
  pthread_cond_t cond_not_empty; // Sygnał: "Są nowe dane, można pobierać"
  pthread_cond_t cond_not_full; // Sygnał: "Zrobiło się miejsce, można wstawiać"
} frame_buffer_t;

// --- ZMIENNE GLOBALNE ---
volatile sig_atomic_t keep_running = 1;

frame_buffer_t left_buf;
frame_buffer_t right_buf;
frame_buffer_t sync_buf;

robot_state_t global_robot_state;
pthread_mutex_t robot_mutex = PTHREAD_MUTEX_INITIALIZER;

// Statystyki
long stat_frames_generated = 0;
long stat_sync_pairs = 0;
long stat_robot_data = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- OBSŁUGA SYGNAŁU CTRL+C ---
void handle_sigint(int sig) {
  printf("\n[SYSTEM] Otrzymano sygnał CTRL+C. Trwa bezpieczne zamykanie...\n");
  keep_running = 0;
}

// --- FUNKCJE POMOCNICZE CZASU ---
double diff_ms(struct timespec *start, struct timespec *end) {
  double start_ms = (start->tv_sec * 1000.0) + (start->tv_nsec / 1000000.0);
  double end_ms = (end->tv_sec * 1000.0) + (end->tv_nsec / 1000000.0);
  return end_ms - start_ms;
}

void sleep_ms(int ms) { usleep(ms * 1000); }

// --- FUNKCJE BUFORA CYKLICZNEGO ---
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

// Wrzucanie do bufora
bool buffer_push(frame_buffer_t *buf, camera_frame_t frame) {
  pthread_mutex_lock(&buf->mutex);

  // Zmienne warunkowe zawsze sprawdzamy w pętli while!
  while (buf->count == buf->capacity && keep_running) {
    pthread_cond_wait(&buf->cond_not_full, &buf->mutex);
  }

  if (!keep_running) { // Jeśli zamykamy program, przerywamy
    pthread_mutex_unlock(&buf->mutex);
    return false;
  }

  buf->buffer[buf->tail] = frame;
  buf->tail = (buf->tail + 1) % buf->capacity;
  buf->count++;

  pthread_cond_signal(
      &buf->cond_not_empty); // Budzi wątek, który czeka na pobranie
  pthread_mutex_unlock(&buf->mutex);
  return true;
}

// Pobieranie z bufora
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
      &buf->cond_not_full); // Budzi wątek, który chce wstawić, ale miał pełno
  pthread_mutex_unlock(&buf->mutex);
  return true;
}

// --- WĄTKI KAMER (~25 Hz) ---
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

// --- WĄTEK SYNCHRONIZACJI ---
void *sync_thread(void *arg) {
  camera_frame_t left, right;

  while (keep_running) {
    // Pobierz klatki z obu buforów
    if (!buffer_pop(&left_buf, &left))
      break;
    if (!buffer_pop(&right_buf, &right))
      break;

    double diff = diff_ms(&left.timestamp, &right.timestamp);
    if (diff < 0)
      diff = -diff;

    if (diff < 20.0) { // Tolerancja 20 ms
      // Zapisujemy tylko numer lewej do bufora synchronizacji dla uproszczenia
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

// --- WĄTEK ZAPISU OBRAZÓW (~10 Hz) ---
void *save_thread(void *arg) {
  camera_frame_t sync_frame;
  while (keep_running) {
    if (buffer_pop(&sync_buf, &sync_frame)) {
      // Symulacja zapisu na dysk
      sleep_ms(100);
    }
  }
  return NULL;
}

// --- WĄTEK STANU ROBOTA (~100 Hz) ---
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

// --- WĄTEK STATYSTYK ---
void *stats_thread(void *arg) {
  while (keep_running) {
    sleep_ms(2000); // Wyświetlaj co 2 sekundy
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

// --- FUNKCJA GŁÓWNA ---
int main() {
  // Podpięcie sygnału CTRL+C
  signal(SIGINT, handle_sigint);

  printf("Uruchamianie systemu poziomu 2...\n");
  printf("Nacisnij CTRL+C aby bezpiecznie zamknac program.\n\n");

  // Inicjalizacja buforów (pojemność np. 10 elementów)
  buffer_init(&left_buf, 10);
  buffer_init(&right_buf, 10);
  buffer_init(&sync_buf, 10);

  pthread_t t_cam_l, t_cam_r, t_sync, t_save, t_robot, t_stats;

  // Przekazujemy adresy buforów jako argumenty do wątków kamer
  pthread_create(&t_cam_l, NULL, camera_thread, &left_buf);
  pthread_create(&t_cam_r, NULL, camera_thread, &right_buf);

  pthread_create(&t_sync, NULL, sync_thread, NULL);
  pthread_create(&t_save, NULL, save_thread, NULL);
  pthread_create(&t_robot, NULL, robot_thread, NULL);
  pthread_create(&t_stats, NULL, stats_thread, NULL);

  // Czekamy na zakończenie wątków (nastąpi to po naciśnięciu CTRL+C)
  // Jednak musimy wybudzić wątki, które utknęły czekając na dane!
  pthread_join(t_stats, NULL); // Ten wątek na pewno się zamknie

  // Budzenie wszystkich "śpiących" wątków w buforach
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

  // Sprzątanie pamięci
  buffer_destroy(&left_buf);
  buffer_destroy(&right_buf);
  buffer_destroy(&sync_buf);

  printf("System poprawnie zamkniety. Do widzenia!\n");
  return 0;
}