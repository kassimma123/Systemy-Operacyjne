#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) { // semafory POSIX
  sem_t *id_sem = sem_open("nazwa_sem", O_CREAT, 0600, 1);
  pid_t pid1 = fork();
  if (pid1 == 0) {
    sleep(1);
    printf("czekam potomny\n");
    fflush(stdout);
    sem_wait(id_sem);
    printf("wewnatrz potomny\n");
    fflush(stdout);
    sleep(3);
    sem_post(id_sem);
    printf("po potomny/n");
    fflush(stdout);
    return 0;
  } else {
    sleep(2);
    printf("czekam główny\n");
    fflush(stdout);
    sem_wait(id_sem);
    printf("wewnatrz glowny\n");
    fflush(stdout);
    sleep(2);
    sem_post(id_sem);
    printf("po główny/n");
    fflush(stdout);
  }
  sem_close(id_sem);
  return 0;
}
