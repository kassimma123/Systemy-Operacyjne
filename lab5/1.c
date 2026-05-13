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

#define K 5             // Rozmiar bufora
#define TASK_LEN 11     // 10 znaków + znak końca stringa '\0'
#define N_PRODUCERS 2   // Liczba producentów
#define M_CONSUMERS 2   // Liczba konsumentów

typedef struct {
    char tasks[K][TASK_LEN];
    int head; //miejsce gdzie jest dodawane nowe zadanie
    int tail; //miejsce gdzie jest pobierane zadanie
}SharedBuffer;

void generate_task(char *buffer){
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 10; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[10] = '\0';
}

int main(){
    //Inicjalizacja Pamięci Wspólnej
    int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedBuffer));
    SharedBuffer *buffer = mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    buffer-> head = 0;
    buffer-> tail = 0;

    //Inicjalizacja semaforów
    sem_unlink("/sem_mutex");
    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");

    sem_t *mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1); //synchronizacja dostepu do bufora
    sem_t *empty = sem_open("/sem_empty", O_CREAT, 0666, K); //liczba pustych miejsc
    sem_t *full  = sem_open("/sem_full",  O_CREAT, 0666, 0); //liczba pelnych miejsc

    for (int i = 0; i < N_PRODUCERS; i++){
        if (fork() == 0){ 
            srand(time(NULL) ^ (getpid() << 16));
            while(1){
                char new_task[TASK_LEN];
                generate_task(new_task);

                sem_wait(empty);
                sem_wait(mutex);

                strcpy(buffer->tasks[buffer->head], new_task);
                printf("[Producent %d] Wygenerowano zadanie: %s na pozycji %d\n", getpid(), new_task, buffer->head);
                buffer->head = (buffer->head + 1) % K;

                sem_post(mutex);
                sem_post(full);

                sleep(1);
                
            }
            exit(0);
        }
    }

    for (int i = 0; i < M_CONSUMERS; i++){
        if (fork() == 0){
            while(1){
                char my_task[TASK_LEN];

                sem_wait(full);
                sem_wait(mutex);

                strcpy(my_task, buffer->tasks[buffer->tail]);
                printf("[Konsument %d] Pobrano zadanie z pozycji %d\n", getpid(), buffer->tail);
                buffer->tail = (buffer->tail + 1) % K;

                sem_post(mutex);
                sem_post(empty);

                printf("[Konsument %d] Przetwarzanie: ", getpid());
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

    for (int i = 0; i < N_PRODUCERS + M_CONSUMERS; i++) {
        wait(NULL);
    }

    munmap(buffer, sizeof(SharedBuffer));
    shm_unlink("/my_shm");
    sem_close(mutex); sem_close(empty); sem_close(full);
    sem_unlink("/sem_mutex"); sem_unlink("/sem_empty"); sem_unlink("/sem_full");

    return 0;
}