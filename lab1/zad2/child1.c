#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    if (argc != 2) return 1;

    int M = atoi(argv[1]);

    for (int i = 0; i < M; i++){
        printf("Potomek (PID: %d)\n", getpid());
        usleep(250000);
    }
    return 0;
}