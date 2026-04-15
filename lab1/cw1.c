#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>



int main (void){
    pid_t p;
    int i;
    p = fork();
    if (p == 0){
        p = fork();
        if (p == 0){
            return 0;
        }else{
            //proces mac
        }

    }
    while(wait(0) > 0){ //czeka na zakończenie procesu potomnego (-1 --> nie czekamy na proces potomny)
        //proces rodzic po zakonczenczeniu potomnych
        return 0;
    }
}