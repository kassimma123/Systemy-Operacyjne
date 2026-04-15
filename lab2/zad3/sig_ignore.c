#include <stdio.h>
#include <signal.h>
#include "signals_lib.h"

void sig_ignore()
{
    signal(SIGUSR1, SIG_IGN);
    printf("Ustawiono reakcje: ignore\n");
}