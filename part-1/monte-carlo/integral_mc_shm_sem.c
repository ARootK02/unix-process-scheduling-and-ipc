#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

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
     * Shared memory stores the final result so all child processes can
     * contribute to the same value.
     */
    double *shared_result = mmap(
        NULL,
        sizeof(double),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (shared_result == MAP_FAILED) {
        perror("mmap shared_result");
        return 1;
    }

    *shared_result = 0.0;

    /*
     * The semaphore is also placed in shared memory so it can synchronize
     * updates between different processes.
     */
    sem_t *sem = mmap(
        NULL,
        sizeof(sem_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (sem == MAP_FAILED) {
        perror("mmap semaphore");
        munmap(shared_result, sizeof(double));
        return 1;
    }

    if (sem_init(sem, 1, 1) != 0) {
        perror("sem_init");
        munmap(shared_result, sizeof(double));
        munmap(sem, sizeof(sem_t));
        return 1;
    }

    t0 = get_wtime();

    for (int i = 0; i < PROCESS_COUNT; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            sem_destroy(sem);
            munmap(shared_result, sizeof(double));
            munmap(sem, sizeof(sem_t));
            return 1;
        }

        if (pid == 0) {
            srand48(tseed + i);

            double local_sum = 0.0;

            /*
             * Each child process computes a different subset of the
             * Monte Carlo samples.
             */
            for (unsigned long j = i; j < n; j += PROCESS_COUNT) {
                double xi = a + (b - a) * drand48();
                local_sum += f(xi);
            }

            double local_result = local_sum * h;

            /*
             * Critical section: only one process at a time may update
             * the shared result.
             */
            sem_wait(sem);
            *shared_result += local_result;
            sem_post(sem);

            _exit(0);
        }
    }

    for (int i = 0; i < PROCESS_COUNT; i++) {
        wait(NULL);
    }

    res = *shared_result;

    t1 = get_wtime();

    printf("Result=%.16f Error=%e Rel.Error=%e Time=%lf seconds\n",
           res, fabs(res - ref), fabs(res - ref) / ref, t1 - t0);

    sem_destroy(sem);
    munmap(shared_result, sizeof(double));
    munmap(sem, sizeof(sem_t));

    return 0;
}