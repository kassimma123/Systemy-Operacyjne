#include <stdio.h>
#include <signal.h>
#include "signals_lib.h"

void handler(int sig)
{
    printf("Wywołano handler dla sygnału %d\n", sig);
}

void sig_handle()
{
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);
    printf("Ustawiono reakcje: handle\n");
}