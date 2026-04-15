#include "definitions.h"
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;
    int M = atoi(argv[1]);

    FILE *plik = fopen(OUTPUT_FILE, "a");
    if (!plik)
    {
        perror("Blad otwarcia pliku");
        return 1;
    }

    int fd = fileno(plik);

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    fcntl(fd, F_SETLKW, &fl);

    for (int i = 0; i < M; i++)
    {
        fprintf(plik, "Potomek (PID: %d)\n", getpid());
        fflush(plik);
        usleep(250000);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    fclose(plik);
    return 0;
}