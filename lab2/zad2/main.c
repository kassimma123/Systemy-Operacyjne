#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    int mode = -1;
    if (strcmp(argv[1], "default") == 0) mode = 0;
    else if (strcmp(argv[1], "ignore") == 0) mode = 1;
    else if (strcmp(argv[1], "mask") == 0) mode = 2;
    else if (strcmp(argv[1], "handle") == 0) mode = 3;
    else {
        fprintf(stderr, "nieznany argument: %s\n", argv[1]);
        return 1;
    }

    pid_t pid = fork(); //klonowanie procesu
    if(pid < 0){
        perror("fork failed");
        return 1;
    }else if (pid == 0){
        execl("./child", "./child", NULL); //uruchomienie programu potomnego
        perror("exec failed");
        exit(1);
    } else { //proces rodzica
        sleep(1);
        union sigval value;
        value.sival_int = mode;
        
        printf("rodzic: wysyłam sygnał SIGUSR2 z konfiguracja (%s = %d) do %d\n", argv[1], mode, pid);
        sigqueue(pid, SIGUSR2, value);

        wait(NULL);
        printf("rodzic: potomek zakończył działanie\n");
    }

    return 0;
}
