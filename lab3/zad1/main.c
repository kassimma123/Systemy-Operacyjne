#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

double f(double x)
{
    return 4.0 / (x * x + 1.0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uzycie: %s <szerokosc_prostokata_dx> <max_liczba_procesow_n>\n", argv[0]);
        fprintf(stderr, "Przyklad: %s 0.000000002 4\n", argv[0]);
        return 1;
    }

    double dx = atof(argv[1]);
    int n = atoi(argv[2]);

    if (dx <= 0 || n <= 0)
    {
        fprintf(stderr, "Szerokosc prostokata dx musi byc dodatnia, a liczba procesow n musi byc dodatnia.\n");
        return 1;
    }

    printf("Rozpoczecie obliczen (dx = %g, max procesow = %d)...\n\n", dx, n);

    for (int k = 1; k <= n; k++)
    {
        int pipes[k][2];

        for (int i = 0; i < k; i++)
        {
            if (pipe(pipes[i]) == -1)
            {
                perror("Blad tworzenia potoku");
                return 1;
            }
        }

        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        for (int i = 0; i < k; i++)
        {
            pid_t pid = fork();

            if (pid < 0)
            {
                perror("Błąd fork");
                exit(1);
            }

            if (pid == 0)
            {
                for (int j = 0; j < k; j++)
                {
                    close(pipes[j][0]);
                    if (j != i)
                    {
                        close(pipes[j][1]);
                    }
                }

                double start_x = (double)i / k;
                double end_x = (double)(i + 1) / k;
                double potential_sum = 0.0;

                for (double x = start_x; x < end_x; x += dx)
                {
                    potential_sum += f(x) * dx;
                }

                if (write(pipes[i][1], &potential_sum, sizeof(potential_sum)) == -1)
                {
                    perror("Blad zapisu do potoku");
                    exit(1);
                }
                close(pipes[i][1]);
                exit(0);
            }
        }
        double total_sum = 0.0;

        for (int i = 0; i < k; i++)
        {
            close(pipes[i][1]);

            double partial_result;
            if (read(pipes[i][0], &partial_result, sizeof(partial_result)) == -1)
            {
                perror("Blad odczytu z potoku");
                exit(1);
            }
            total_sum += partial_result;

            close(pipes[i][0]);
        }

        for (int i = 0; i < k; i++)
        {
            wait(NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &end_time);

        double time_taken = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        printf("Liczba procesow (k): %d | Wynik: %.10f | Czas: %.4f s\n", k, total_sum, time_taken);
    }
    return 0;
}