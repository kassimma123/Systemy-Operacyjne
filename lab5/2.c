#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>

#define K 5             // Rozmiar KAŻDEJ z kolejek
#define TASK_LEN 11     
#define N_PRODUCERS 2   
#define M_CONSUMERS 2   

// Rozbudowana struktura pamięci współdzielonej
typedef struct {
    char normal_tasks[K][TASK_LEN];
    int normal_head;
    int normal_tail;
    int normal_count; // Liczba elementów w NORMAL

    char prio_tasks[K][TASK_LEN];
    int prio_head;
    int prio_tail;
    int prio_count;   // Liczba elementów w PRIORITY
} SharedBuffer;

void generate_task(char *buffer) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 10; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[10] = '\0';
}

int main() {
    //Inicjalizacja Pamięci Wspólnej
    int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedBuffer));
    SharedBuffer *buffer = mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    //Wyzerowanie struktury
    memset(buffer, 0, sizeof(SharedBuffer));

    //Inicjalizacja Semaforów
    sem_unlink("/sem_mutex");
    sem_unlink("/sem_items");
    sem_unlink("/sem_enorm");
    sem_unlink("/sem_eprio");

    sem_t *mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1); //synchronizacja dostepu do bufora
    sem_t *items_avail = sem_open("/sem_items", O_CREAT, 0666, 0); // Liczba wszystkich zadan
    sem_t *empty_norm = sem_open("/sem_enorm", O_CREAT, 0666, K);  // Puste miejsca w NORMAL
    sem_t *empty_prio = sem_open("/sem_eprio", O_CREAT, 0666, K);  // Puste miejsca w PRIORITY

    //Tworzenie procesów Producentów
    for (int i = 0; i < N_PRODUCERS; i++) {
        if (fork() == 0) {
            srand(time(NULL) ^ (getpid() << 16)); 
            while (1) {
                char new_task[TASK_LEN];
                generate_task(new_task);

                //losowanie: 30% szans na PRIORITY, 70% na NORMAL
                int is_priority = (rand() % 100) < 30;

                if (is_priority) {
                    sem_wait(empty_prio); 
                    sem_wait(mutex);      

                    strcpy(buffer->prio_tasks[buffer->prio_head], new_task);
                    buffer->prio_head = (buffer->prio_head + 1) % K;
                    buffer->prio_count++;
                    printf("[Producent %d] Wygenerowano zadanie PRIORITY: %s\n", getpid(), new_task);

                    sem_post(mutex);
                } else {
                    sem_wait(empty_norm); 
                    sem_wait(mutex);      

                    strcpy(buffer->normal_tasks[buffer->normal_head], new_task);
                    buffer->normal_head = (buffer->normal_head + 1) % K;
                    buffer->normal_count++;
                    printf("[Producent %d] Wygenerowano zadanie NORMAL: %s\n", getpid(), new_task);

                    sem_post(mutex);
                }

                // Bez względu na to gdzie dodaliśmy, zgłaszamy, że jest 1 nowe zadanie w systemie
                sem_post(items_avail); 
                sleep(1); 
            }
            exit(0);
        }
    }

    //Tworzenie procesów Konsumentów
    for (int i = 0; i < M_CONSUMERS; i++) {
        if (fork() == 0) {
            while (1) {
                char my_task[TASK_LEN];
                int is_prio_task = 0;

                //czekaj na jakikolwiek zadanie (prio lub normal)
                sem_wait(items_avail); 
                sem_wait(mutex); 

                //sprawdzamy, skąd pobrać zadanie
                if (buffer->prio_count > 0) {
                    strcpy(my_task, buffer->prio_tasks[buffer->prio_tail]);
                    buffer->prio_tail = (buffer->prio_tail + 1) % K;
                    buffer->prio_count--;
                    is_prio_task = 1;
                    
                    sem_post(mutex);
                    sem_post(empty_prio); // Zwolniliśmy miejsce w PRIORITY
                } else {
                    strcpy(my_task, buffer->normal_tasks[buffer->normal_tail]);
                    buffer->normal_tail = (buffer->normal_tail + 1) % K;
                    buffer->normal_count--;
                    
                    sem_post(mutex);
                    sem_post(empty_norm); // Zwolniliśmy miejsce w NORMAL
                }

                // Przetwarzanie (poza sekcją krytyczną!)
                printf("[Konsument %d] Rozpoczynam %s: ", getpid(), is_prio_task ? "PRIORITY" : "NORMAL");
                for (int j = 0; j < 10; j++) {
                    putchar(my_task[j]);
                    fflush(stdout); 
                    usleep(300000); 
                }
                printf("\n");
            }
            exit(0);
        }
    }

    // Oczekiwanie na procesy
    for (int i = 0; i < N_PRODUCERS + M_CONSUMERS; i++) {
        wait(NULL);
    }

    // Sprzątanie
    munmap(buffer, sizeof(SharedBuffer));
    shm_unlink("/my_shm");
    sem_close(mutex); sem_close(items_avail); sem_close(empty_norm); sem_close(empty_prio);
    sem_unlink("/sem_mutex"); sem_unlink("/sem_items"); sem_unlink("/sem_enorm"); sem_unlink("/sem_eprio");

    return 0;
}