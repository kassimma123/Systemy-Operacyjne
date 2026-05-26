#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// --- STRUKTURY DANYCH ---
typedef struct {
    unsigned int frame_num;
    struct timespec timestamp;
} camera_frame_t;

typedef struct {
    double x, y, z;
    struct timespec timestamp;
} robot_state_t;

// --- ZMIENNE GLOBALNE I MECHANIZMY SYNCHRONIZACJI ---
volatile bool keep_running = true;

// Lewa kamera
camera_frame_t global_left_frame;
pthread_mutex_t left_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t left_sem;

// Prawa kamera
camera_frame_t global_right_frame;
pthread_mutex_t right_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t right_sem;

// Zsynchronizowane klatki do zapisu
camera_frame_t sync_left_frame;
camera_frame_t sync_right_frame;
pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sync_sem;

// Stan robota
robot_state_t global_robot_state;
pthread_mutex_t robot_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- FUNKCJE POMOCNICZE ---
double diff_ms(struct timespec *start, struct timespec *end) {
    double start_ms = (start->tv_sec * 1000.0) + (start->tv_nsec / 1000000.0);
    double end_ms = (end->tv_sec * 1000.0) + (end->tv_nsec / 1000000.0);
    return end_ms - start_ms;
}

void sleep_ms(int ms) {
    usleep(ms * 1000);
}

// --- WĄTKI KAMER (25 Hz = co 40 ms) ---
void* left_camera_thread(void* arg) {
    unsigned int counter = 0;
    while (keep_running) {
        camera_frame_t frame;
        frame.frame_num = ++counter;
        clock_gettime(CLOCK_REALTIME, &frame.timestamp);

        pthread_mutex_lock(&left_mutex);
        global_left_frame = frame; // Zapis do zmiennej współdzielonej
        pthread_mutex_unlock(&left_mutex);
        
        sem_post(&left_sem); // Sygnalizacja: "Nowa klatka!"
        sleep_ms(40);
    }
    return NULL;
}

void* right_camera_thread(void* arg) {
    unsigned int counter = 0;
    while (keep_running) {
        camera_frame_t frame;
        frame.frame_num = ++counter;
        clock_gettime(CLOCK_REALTIME, &frame.timestamp);

        pthread_mutex_lock(&right_mutex);
        global_right_frame = frame;
        pthread_mutex_unlock(&right_mutex);
        
        sem_post(&right_sem);
        sleep_ms(40);
    }
    return NULL;
}

// --- WĄTEK SYNCHRONIZACJI ---
void* sync_thread(void* arg) {
    while (keep_running) {
        // Czekaj na sygnał od OBU kamer
        sem_wait(&left_sem);
        sem_wait(&right_sem);

        camera_frame_t left, right;

        // Pobierz bezpiecznie klatki
        pthread_mutex_lock(&left_mutex);
        left = global_left_frame;
        pthread_mutex_unlock(&left_mutex);

        pthread_mutex_lock(&right_mutex);
        right = global_right_frame;
        pthread_mutex_unlock(&right_mutex);

        // Sprawdź różnicę czasu
        double diff = diff_ms(&left.timestamp, &right.timestamp);
        if (diff < 0) diff = -diff; // Wartość bezwzględna

        if (diff < 20.0) { // Tolerancja 20 ms
            pthread_mutex_lock(&sync_mutex);
            sync_left_frame = left;
            sync_right_frame = right;
            pthread_mutex_unlock(&sync_mutex);
            
            sem_post(&sync_sem); // Przekaż do zapisu
        }
    }
    return NULL;
}

// --- WĄTEK ZAPISU OBRAZÓW (10 Hz = co 100 ms) ---
void* save_thread(void* arg) {
    while (keep_running) {
        // Czekaj na zsynchronizowaną parę
        if (sem_wait(&sync_sem) == 0) {
            pthread_mutex_lock(&sync_mutex);
            unsigned int l_num = sync_left_frame.frame_num;
            unsigned int r_num = sync_right_frame.frame_num;
            pthread_mutex_unlock(&sync_mutex);

            printf("[KAMERY] Zapisano pare stereo: left_%04u.jpg, right_%04u.jpg\n", l_num, r_num);
            sleep_ms(100); // Wymuś max 10 Hz zapisu
        }
    }
    return NULL;
}

// --- WĄTEK STANU ROBOTA (100 Hz = co 10 ms) ---
void* robot_thread(void* arg) {
    double pos = 0.0;
    while (keep_running) {
        pthread_mutex_lock(&robot_mutex);
        global_robot_state.x = pos;
        global_robot_state.y = pos * 2;
        clock_gettime(CLOCK_REALTIME, &global_robot_state.timestamp);
        pthread_mutex_unlock(&robot_mutex);

        pos += 0.1;
        sleep_ms(10);
    }
    return NULL;
}

// --- WĄTEK LOGGERA (10 Hz = co 100 ms) ---
void* logger_thread(void* arg) {
    while (keep_running) {
        pthread_mutex_lock(&robot_mutex);
        double x = global_robot_state.x;
        double y = global_robot_state.y;
        pthread_mutex_unlock(&robot_mutex);

        printf("[ROBOT ] Pozycja: X=%.2f, Y=%.2f\n", x, y);
        sleep_ms(100);
    }
    return NULL;
}

// --- FUNKCJA GŁÓWNA ---
int main() {
    printf("Uruchamianie systemu (Zadanie 1)...\n");

    // Inicjalizacja semaforów
    sem_init(&left_sem, 0, 0);
    sem_init(&right_sem, 0, 0);
    sem_init(&sync_sem, 0, 0);

    // Identyfikatory wątków
    pthread_t t_cam_l, t_cam_r, t_sync, t_save, t_robot, t_logger;

    // Tworzenie wątków
    pthread_create(&t_cam_l, NULL, left_camera_thread, NULL);
    pthread_create(&t_cam_r, NULL, right_camera_thread, NULL);
    pthread_create(&t_sync, NULL, sync_thread, NULL);
    pthread_create(&t_save, NULL, save_thread, NULL);
    pthread_create(&t_robot, NULL, robot_thread, NULL);
    pthread_create(&t_logger, NULL, logger_thread, NULL);

    // Symulacja działania przez 5 sekund (możesz zmienić na 20)
    sleep(5);

    printf("\nZamykanie systemu...\n");
    keep_running = false;

    // Odblokowanie semaforów, aby wątki mogły się zakończyć
    sem_post(&left_sem);
    sem_post(&right_sem);
    sem_post(&sync_sem);

    // Czekanie na zakończenie wątków
    pthread_join(t_cam_l, NULL);
    pthread_join(t_cam_r, NULL);
    pthread_join(t_sync, NULL);
    pthread_join(t_save, NULL);
    pthread_join(t_robot, NULL);
    pthread_join(t_logger, NULL);

    // Sprzątanie pamięci
    sem_destroy(&left_sem);
    sem_destroy(&right_sem);
    sem_destroy(&sync_sem);

    printf("System poprawnie zamkniety.\n");
    return 0;
}