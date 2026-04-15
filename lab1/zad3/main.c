#include "definitions.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
        return 1;

    int N = atoi(argv[1]);
    char *M_str = argv[2];

    remove(OUTPUT_FILE);

    for (int i = 0; i < N; i++)
    {
        if (fork() == 0)
        {
            execl("./child", "child", M_str, NULL);
        }
    }

    while (wait(NULL) > 0)
        ;
    printf("Rodzic zakonczyl (PID: %d)\n", getpid());

    return 0;
}