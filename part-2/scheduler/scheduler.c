#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_PROCESSES 128
#define MAX_CMD_LEN 256

typedef struct {
    char command[MAX_CMD_LEN];
    pid_t pid;
    int finished;
    double start_time;
    double finish_time;
} Process;

double get_wtime(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

void remove_newline(char *str) {
    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

int load_processes(const char *filename, Process processes[]) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    int count = 0;
    char line[MAX_CMD_LEN];

    /*
     * Each non-empty line in the workload file is treated as one
     * executable command for the scheduler.
     */
    while (fgets(line, sizeof(line), file) != NULL && count < MAX_PROCESSES) {
        remove_newline(line);

        if (strlen(line) == 0) {
            continue;
        }

        strncpy(processes[count].command, line, MAX_CMD_LEN - 1);
        processes[count].command[MAX_CMD_LEN - 1] = '\0';
        processes[count].pid = -1;
        processes[count].finished = 0;
        processes[count].start_time = 0.0;
        processes[count].finish_time = 0.0;

        count++;
    }

    fclose(file);
    return count;
}

void run_fcfs(Process processes[], int count) {
    double scheduler_start = get_wtime();

    /*
     * FCFS runs each process to completion before starting the next one.
     */
    for (int i = 0; i < count; i++) {
        printf("executing %s\n", processes[i].command);

        processes[i].start_time = get_wtime();

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            execl(processes[i].command, processes[i].command, NULL);
            perror("execl");
            _exit(EXIT_FAILURE);
        }

        processes[i].pid = pid;

        int status;
        waitpid(pid, &status, 0);

        processes[i].finish_time = get_wtime();
        processes[i].finished = 1;

        printf("PID %d - CMD: %s\n", processes[i].pid, processes[i].command);
        printf("\tElapsed time = %.2f secs\n", processes[i].finish_time - scheduler_start);
        printf("\tWorkload time = %.2f secs\n", processes[i].finish_time - processes[i].start_time);
    }

    double scheduler_end = get_wtime();

    printf("WORKLOAD TIME: %.2f secs\n", scheduler_end - scheduler_start);
    printf("scheduler exits\n");
}

void start_all_stopped(Process processes[], int count) {
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            /*
             * Start each child in a stopped state so the scheduler can
             * later resume it according to the selected policy.
             */
            raise(SIGSTOP);
            execl(processes[i].command, processes[i].command, NULL);
            perror("execl");
            _exit(EXIT_FAILURE);
        }

        processes[i].pid = pid;
        processes[i].start_time = get_wtime();

        int status;
        waitpid(pid, &status, WUNTRACED);
    }
}

void run_rr(Process processes[], int count, int quantum_ms) {
    int finished_count = 0;
    double scheduler_start = get_wtime();

    start_all_stopped(processes, count);

    /*
     * Round Robin repeatedly resumes each unfinished process for one
     * time quantum, then stops it again if it has not completed.
     */
    while (finished_count < count) {
        for (int i = 0; i < count; i++) {
            if (processes[i].finished) {
                continue;
            }

            printf("running %s with PID %d\n", processes[i].command, processes[i].pid);

            if (kill(processes[i].pid, SIGCONT) == -1) {
                if (errno == ESRCH) {
                    processes[i].finished = 1;
                    finished_count++;
                    continue;
                }

                perror("kill SIGCONT");
                exit(EXIT_FAILURE);
            }

            usleep((useconds_t)quantum_ms * 1000);

            int status;
            pid_t result = waitpid(processes[i].pid, &status, WNOHANG);

            if (result == processes[i].pid) {
                processes[i].finish_time = get_wtime();
                processes[i].finished = 1;
                finished_count++;

                printf("PID %d - CMD: %s\n", processes[i].pid, processes[i].command);
                printf("\tElapsed time = %.2f secs\n", processes[i].finish_time - scheduler_start);
                printf("\tWorkload time = %.2f secs\n", processes[i].finish_time - processes[i].start_time);
            } else if (result == 0) {
                /*
                 * The process is still running after its quantum, so it is
                 * preempted with SIGSTOP.
                 */
                if (kill(processes[i].pid, SIGSTOP) == -1) {
                    if (errno == ESRCH) {
                        processes[i].finish_time = get_wtime();
                        processes[i].finished = 1;
                        finished_count++;
                    } else {
                        perror("kill SIGSTOP");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    waitpid(processes[i].pid, &status, WUNTRACED);
                }
            } else {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }
    }

    double scheduler_end = get_wtime();

    printf("WORKLOAD TIME: %.2f secs\n", scheduler_end - scheduler_start);
    printf("scheduler exits\n");
}

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s FCFS <workload_file>\n", program_name);
    fprintf(stderr, "  %s RR <quantum_ms> <workload_file>\n", program_name);
}

int main(int argc, char **argv) {
    Process processes[MAX_PROCESSES];
    int process_count;

    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "FCFS") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        process_count = load_processes(argv[2], processes);

        if (process_count <= 0) {
            fprintf(stderr, "No processes loaded.\n");
            return EXIT_FAILURE;
        }

        run_fcfs(processes, process_count);
    } else if (strcmp(argv[1], "RR") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        int quantum_ms = atoi(argv[2]);

        if (quantum_ms <= 0) {
            fprintf(stderr, "Quantum must be a positive number of milliseconds.\n");
            return EXIT_FAILURE;
        }

        process_count = load_processes(argv[3], processes);

        if (process_count <= 0) {
            fprintf(stderr, "No processes loaded.\n");
            return EXIT_FAILURE;
        }

        run_rr(processes, process_count, quantum_ms);
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}