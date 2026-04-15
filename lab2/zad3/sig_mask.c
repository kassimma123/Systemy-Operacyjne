#include <stdio.h>
#include <signal.h>
#include "signals_lib.h"

void sig_mask() {
    sigset_t newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &newmask, NULL);
    printf("Ustawiono reakcje: mask\n");
}

void sig_unblock() {
    sigset_t unblockmask;
    sigemptyset(&unblockmask);
    sigaddset(&unblockmask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);
}