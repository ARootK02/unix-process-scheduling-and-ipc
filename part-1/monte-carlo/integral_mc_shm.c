#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROCESS_COUNT 4

double get_wtime(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

static inline double f(double x) {
    return sin(cos(x));
}

int main(int argc, char *argv[]) {
    const double a = 0.0;
    const double b = 1.0;
    const double ref = 0.73864299803689018;
    const long tseed = 10;

    unsigned long n = 240000000UL;
    double res = 0.0;
    double t0, t1;

    if (argc == 2) {
        n = strtoul(argv[1], NULL, 10);

        if (n == 0) {
            fprintf(stderr, "Usage: %s [number_of_samples]\n", argv[0]);
            return 1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [number_of_samples]\n", argv[0]);
        return 1;
    }

    const double h = (b - a) / (double)n;

    /*
     * Each child process writes its partial result to a separate position
     * in shared memory. This avoids the need for synchronization.
     */
    double *partial_results = mmap(
        NULL,
        PROCESS_COUNT * sizeof(double),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (partial_results == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    for (int i = 0; i < PROCESS_COUNT; i++) {
        partial_results[i] = 0.0;
    }

    t0 = get_wtime();

    for (int i = 0; i < PROCESS_COUNT; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            munmap(partial_results, PROCESS_COUNT * sizeof(double));
            return 1;
        }

        if (pid == 0) {
            srand48(tseed + i);

            double local_sum = 0.0;

            /*
             * Child processes split the Monte Carlo samples in a round-robin
             * pattern based on their process index.
             */
            for (unsigned long j = i; j < n; j += PROCESS_COUNT) {
                double xi = a + (b - a) * drand48();
                local_sum += f(xi);
            }

            partial_results[i] = local_sum * h;
            _exit(0);
        }
    }

    /*
     * The parent waits until all child processes have written their
     * partial results.
     */
    for (int i = 0; i < PROCESS_COUNT; i++) {
        wait(NULL);
    }

    for (int i = 0; i < PROCESS_COUNT; i++) {
        res += partial_results[i];
    }

    t1 = get_wtime();

    printf("Result=%.16f Error=%e Rel.Error=%e Time=%lf seconds\n",
           res, fabs(res - ref), fabs(res - ref) / ref, t1 - t0);

    munmap(partial_results, PROCESS_COUNT * sizeof(double));

    return 0;
}