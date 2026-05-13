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

#define K 5             
#define TASK_LEN 11     
#define N_PRODUCERS 2   
#define M_CONSUMERS 2   

typedef struct {
    char normal_tasks[K][TASK_LEN];
    int normal_head;
    int normal_tail;
    int normal_count; 

    char prio_tasks[K][TASK_LEN];
    int prio_head;
    int prio_tail;
    int prio_count;   
} SharedBuffer;

void generate_task(char *buffer) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 10; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[10] = '\0';
}

int main() {
    int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedBuffer));
    SharedBuffer *buffer = mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    memset(buffer, 0, sizeof(SharedBuffer));

    sem_unlink("/sem_mutex");
    sem_unlink("/sem_items");
    sem_unlink("/sem_enorm");
    sem_unlink("/sem_eprio");

    sem_t *mutex = sem_open("/sem_mutex", O_CREAT, 0666, 1);
    sem_t *items_avail = sem_open("/sem_items", O_CREAT, 0666, 0); 
    sem_t *empty_norm = sem_open("/sem_enorm", O_CREAT, 0666, K);  
    sem_t *empty_prio = sem_open("/sem_eprio", O_CREAT, 0666, K);  

    // PRODUCENCI
    for (int i = 0; i < N_PRODUCERS; i++) {
        if (fork() == 0) {
            srand(time(NULL) ^ (getpid() << 16)); 
            while (1) {
                char new_task[TASK_LEN];
                generate_task(new_task);

                int is_priority = (rand() % 100) < 30;

                if (is_priority) {
                    sem_wait(empty_prio); 
                    sem_wait(mutex);      

                    strcpy(buffer->prio_tasks[buffer->prio_head], new_task);
                    buffer->prio_head = (buffer->prio_head + 1) % K;
                    buffer->prio_count++;
                    printf("[Producent %d] + PRIORITY: %s\n", getpid(), new_task);

                    sem_post(mutex);
                } else {
                    sem_wait(empty_norm); 
                    sem_wait(mutex);      

                    strcpy(buffer->normal_tasks[buffer->normal_head], new_task);
                    buffer->normal_head = (buffer->normal_head + 1) % K;
                    buffer->normal_count++;
                    printf("[Producent %d] + NORMAL:   %s\n", getpid(), new_task);

                    sem_post(mutex);
                }

                sem_post(items_avail); 
                sleep(1 + (rand() % 2)); // Zmienne tempo produkcji
            }
            exit(0);
        }
    }

    // KONSUMENCI
    for (int i = 0; i < M_CONSUMERS; i++) {
        if (fork() == 0) {
            while (1) {
                char my_task[TASK_LEN];
                int is_prio_task = 0;

                sem_wait(items_avail); 
                sem_wait(mutex); 

                if (buffer->prio_count > 0) {
                    strcpy(my_task, buffer->prio_tasks[buffer->prio_tail]);
                    buffer->prio_tail = (buffer->prio_tail + 1) % K;
                    buffer->prio_count--;
                    is_prio_task = 1;
                    
                    sem_post(mutex);
                    sem_post(empty_prio); 
                } else {
                    strcpy(my_task, buffer->normal_tasks[buffer->normal_tail]);
                    buffer->normal_tail = (buffer->normal_tail + 1) % K;
                    buffer->normal_count--;
                    
                    sem_post(mutex);
                    sem_post(empty_norm); 
                }

                printf("[Konsument %d] - Przetwarzam %s: ", getpid(), is_prio_task ? "PRIORITY" : "NORMAL");
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

    // MANAGER 
    if (fork() == 0) {
        while (1) {
            sleep(5); //manager działa co 5 sekund
            
            sem_wait(mutex); //blokujemy dostęp innym na czas monitoringu i modyfikacji

            printf("\n--- [MANAGER] Raport Systemu ---\n");
            printf("Zadań NORMAL: %d/%d\n", buffer->normal_count, K);
            printf("Zadań PRIORITY: %d/%d\n", buffer->prio_count, K);

            //zapobieganie starvation 
            if (buffer->normal_count > 0 && buffer->prio_count < K) {
                sem_trywait(empty_prio); 

                char task_to_move[TASK_LEN];
                strcpy(task_to_move, buffer->normal_tasks[buffer->normal_tail]);
                
                buffer->normal_tail = (buffer->normal_tail + 1) % K;
                buffer->normal_count--;

                strcpy(buffer->prio_tasks[buffer->prio_head], task_to_move);
                buffer->prio_head = (buffer->prio_head + 1) % K;
                buffer->prio_count++;

                sem_post(empty_norm);

                printf(">>> [MANAGER] Przeniesiono zadanie %s z NORMAL do PRIORITY (Starvation Prevention)\n", task_to_move);
            } else if (buffer->normal_count > 0 && buffer->prio_count == K) {
                printf(">>> [MANAGER] Brak miejsca w PRIORITY, nie można przenieść zadań z NORMAL.\n");
            }

            printf("--------------------------------\n\n");

            sem_post(mutex); //odblokowujemy dostęp
        }
        exit(0);
    }

    for (int i = 0; i < N_PRODUCERS + M_CONSUMERS + 1; i++) {
        wait(NULL);
    }

    munmap(buffer, sizeof(SharedBuffer));
    shm_unlink("/my_shm");
    sem_close(mutex); sem_close(items_avail); sem_close(empty_norm); sem_close(empty_prio);
    sem_unlink("/sem_mutex"); sem_unlink("/sem_items"); sem_unlink("/sem_enorm"); sem_unlink("/sem_eprio");

    return 0;
}