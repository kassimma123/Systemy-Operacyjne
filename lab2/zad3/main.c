#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#ifdef DYNAMIC
#include <dlfcn.h>
#else
#include "signals_lib.h"
#endif

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;

#ifdef DYNAMIC
    void *handle = dlopen("./libsignals.so", RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "Błąd ładownia biblioteki: %s\n", dlerror());
        return 1;
    }

    void (*func_ptr)() = NULL;
    void (*func_unblock)() = dlsym(handle, "sig_unblock");

//dynamiczne
    if (strcmp(argv[1], "default") == 0)
        func_ptr = dlsym(handle, "sig_default");
    else if (strcmp(argv[1], "ignore") == 0)
        func_ptr = dlsym(handle, "sig_ignore");
    else if (strcmp(argv[1], "handle") == 0)
        func_ptr = dlsym(handle, "sig_handle");
    else if (strcmp(argv[1], "mask") == 0)
        func_ptr = dlsym(handle, "sig_mask");

    if (func_ptr)
        func_ptr();
        
//statycznie lub wspóldzielone
#else
    if (strcmp(argv[1], "default") == 0)
        sig_default();
    else if (strcmp(argv[1], "ignore") == 0)
        sig_ignore();
    else if (strcmp(argv[1], "handle") == 0)
        sig_handle();
    else if (strcmp(argv[1], "mask") == 0)
        sig_mask();
#endif

    for (int i = 1; i <= 20; i++)
    {
        printf("%d\n", i);
        if (i == 5 || i == 15)
            raise(SIGUSR1);
        if (i == 10)
        {
            sigset_t pending;
            sigpending(&pending);
            if (sigismember(&pending, SIGUSR1))
            {
                printf("Odblokowuję USR1\n");
#ifdef DYNAMIC
                if (func_unblock)
                    func_unblock();
#else
                sig_unblock();
#endif
            }
        }
        sleep(1);
    }

#ifdef DYNAMIC
    dlclose(handle);
#endif
    return 0;
}