#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define M 5 // Ilość wypisań

// Zmienna globalna
int zmiennaGlobalna = 10;

int main(int argc, char *argv[]) {
    // Sprawdzamy czy podano argument (N)
    if (argc != 2) {
        printf("Uzycie: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);

    for (int i = 0; i < N; i++){
        pid_t pid = vfork();

        if (pid == 0){
            zmiennaGlobalna++;
            for (int j = 0; j < M; j++){
                printf("Potomek (PID: %d)\n", getpid());
                usleep(250000);
            }
            exit(0);
        }
    }

    while(wait(NULL)>0);
    printf("Rodzic (PID: %d), Zmienna globalna: %d\n", getpid(), zmiennaGlobalna);
    return 0;
}