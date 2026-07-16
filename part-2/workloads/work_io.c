#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#ifndef LOAD
#define LOAD 5
#endif

#ifndef DELAY
#define DELAY 750
#endif

double a = 1.1;

/*
 * Notifies the parent scheduler that the process entered simulated I/O,
 * waits for the I/O duration, then notifies that I/O completed.
 */
void perform_io(int ms)
{
    kill(getppid(), SIGUSR1);
    usleep((useconds_t)ms * 1000);
    kill(getppid(), SIGUSR2);

    /*
     * Stop after I/O so the scheduler can decide when to resume the process.
     */
    raise(SIGSTOP);
}

/*
 * Performs repeated floating-point operations to create CPU-bound work.
 * The global variable prevents the compiler from removing the calculations.
 */
void core_delay(void)
{
    unsigned long j;

    for (j = 0; j < 100000; j++) {
        a += sqrt(1.1) * sqrt(1.2) * sqrt(1.3) * sqrt(1.4) * sqrt(1.5);
        a += sqrt(1.6) * sqrt(1.7) * sqrt(1.8) * sqrt(1.9) * sqrt(2.0);
        a += sqrt(1.1) * sqrt(1.2) * sqrt(1.3) * sqrt(1.4) * sqrt(1.5);
        a += sqrt(1.6) * sqrt(1.7) * sqrt(1.8) * sqrt(1.9);
    }
}

void delay(int workload)
{
    int i;
    int total_workload = workload * DELAY;

    for (i = 0; i < total_workload; i++) {
        core_delay();
    }
}

int main(void)
{
    int workload = LOAD;
    int pid = getpid();

    printf("process %d begins\n", pid);
    delay(workload);

    printf("process %d starts io\n", pid);
    perform_io(3000);

    printf("process %d completed io\n", pid);
    delay(workload);

    printf("process %d ends\n", pid);

    return 0;
}