#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void handler(int sig)
{
    printf("wywołano handler dla sygnału %d\n", sig);
}

void sig_default()
{
    signal(SIGUSR1, SIG_DFL);
    printf("ustawiono reakcję: default\n");
}

void sig_ignore()
{
    signal(SIGUSR1, SIG_IGN);
    printf("ustawiono reakcję: ignore\n");
}

void sig_handle()
{
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);
    printf("ustawiono reakcję: handle\n");
}

void sig_mask()
{
    sigset_t newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &newmask, NULL);
    printf("ustawiono reakcję: mask\n");
}

void sig_unblock()
{
    sigset_t unblockmask;
    sigemptyset(&unblockmask);
    sigaddset(&unblockmask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "wywołanie: %s default|mask|ignore|handle\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "default") == 0)
    {
        sig_default();
    }
    else if (strcmp(argv[1], "ignore") == 0)
    {
        sig_ignore();
    }
    else if (strcmp(argv[1], "mask") == 0)
    {
        sig_mask();
    }
    else if (strcmp(argv[1], "handle") == 0)
    {
        sig_handle();
    }
    else
    {
        fprintf(stderr, "nieznany argument: %s\n", argv[1]);
        return 1;
    }

    for (int i = 1; i <= 20; i++)
    {
        printf("%d\n", i);

        if (i == 5 || i == 15)
        {
            printf("wysyłam sygnał USR1\n");
            raise(SIGUSR1);
        }

        if (i == 10)
        {
            sigset_t pending;
            sigpending(&pending);

            if (sigismember(&pending, SIGUSR1))
            {
                printf("odblokowuje usr1\n");
                sig_unblock();
            }
        }
        sleep(1);
    }
    printf("petla została wykonana w całości\n");
    return 0;
}