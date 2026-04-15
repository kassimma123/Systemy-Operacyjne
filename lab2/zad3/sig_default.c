#include <stdio.h>
#include <signal.h>
#include "signals_lib.h"

void sig_default()
{
    signal(SIGUSR1, SIG_DFL);
    printf("Ustawiono reakcje: default\n");
}