#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Uzycie: %s <N> <M>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    char *M_str = argv[2];

    for (int i = 0; i < N; i++)
    {
        if (fork() == 0)
        {
            execl(",/child", "child", M_str, NULL);
        }
    }

    while (wait(NULL) > 0)
        ;
    printf("Rodzic (PID: %d)\n", getpid());
    return 0;
}