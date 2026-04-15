#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t config_mode = -1;

void handler(int sig)
{
    printf("wywołano handler dla sygnału %d\n", sig);
}

void sig_default()
{
    signal(SIGUSR1, SIG_DFL);
    printf("potomek: default\n");
}
void sig_ignore()
{
    signal(SIGUSR1, SIG_IGN);
    printf("potomek: ignore\n");
}
void sig_handle()
{
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);
    printf("potomek: handler\n");
}
void sig_mask()
{
    sigset_t newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &newmask, NULL);
    printf("potomek: mask\n");
}
void sig_unblock()
{
    sigset_t unblockmask;
    sigemptyset(&unblockmask);
    sigaddset(&unblockmask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);
}

void usr2_handler(int sig, siginfo_t *info, void *uncontext) //synał i info o sygnale
{
    config_mode = info->si_value.sival_int;
}

int main()
{
    struct sigaction act;
    act.sa_sigaction = usr2_handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR2, &act, NULL);

    printf("potomek (PID: %d) czeka na konfiguracje\n", getpid());

    while (config_mode == -1)
    {
        pause();
    }

    if (config_mode == 0)
        sig_default();
    else if (config_mode == 1)
        sig_ignore();
    else if (config_mode == 2)
        sig_mask();
    else if (config_mode == 3)
        sig_handle();

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
                printf("odblokowuje USR1\n");
                sig_unblock();
            }
        }
        sleep(1);
    }
    printf("pętla została wykonana w całości\n");
    return 0;
}