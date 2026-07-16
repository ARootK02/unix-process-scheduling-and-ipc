#define _GNU_SOURCE

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
    int stopped;
    volatile sig_atomic_t in_io;
    volatile sig_atomic_t io_completed;
    double start_time;
    double finish_time;
} Process;

/*
 * The signal handler needs access to the process table in order to update
 * the state of the process that entered or completed simulated I/O.
 */
static Process *global_processes = NULL;
static int global_process_count = 0;

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
        processes[count].stopped = 0;
        processes[count].in_io = 0;
        processes[count].io_completed = 0;
        processes[count].start_time = 0.0;
        processes[count].finish_time = 0.0;

        count++;
    }

    fclose(file);
    return count;
}

int find_process_by_pid(pid_t pid) {
    for (int i = 0; i < global_process_count; i++) {
        if (global_processes[i].pid == pid) {
            return i;
        }
    }

    return -1;
}

void io_signal_handler(int signal_number, siginfo_t *info, void *context) {
    (void)context;

    if (global_processes == NULL || info == NULL) {
        return;
    }

    int index = find_process_by_pid(info->si_pid);

    if (index == -1) {
        return;
    }

    /*
     * SIGUSR1 marks the process as waiting for simulated I/O.
     * SIGUSR2 marks the process as ready to be scheduled again.
     */
    if (signal_number == SIGUSR1) {
        global_processes[index].in_io = 1;
    } else if (signal_number == SIGUSR2) {
        global_processes[index].in_io = 0;
        global_processes[index].io_completed = 1;
    }
}

void setup_io_signal_handlers(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = io_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

void print_process_finished(Process *process, double scheduler_start) {
    printf("PID %d - CMD: %s\n", process->pid, process->command);
    printf("\tElapsed time = %.2f secs\n", process->finish_time - scheduler_start);
    printf("\tWorkload time = %.2f secs\n", process->finish_time - process->start_time);
}

void run_fcfs_io(Process processes[], int count) {
    double scheduler_start = get_wtime();

    /*
     * FCFS starts one process at a time. If a process stops because of
     * simulated I/O, the scheduler resumes it after the I/O notification.
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

        while (!processes[i].finished) {
            int status;
            pid_t result = waitpid(pid, &status, WUNTRACED);

            if (result == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                processes[i].finish_time = get_wtime();
                processes[i].finished = 1;
                print_process_finished(&processes[i], scheduler_start);
            } else if (WIFSTOPPED(status)) {
                printf("PID %d completed I/O and stopped. Resuming...\n", pid);

                if (kill(pid, SIGCONT) == -1) {
                    perror("kill SIGCONT");
                    exit(EXIT_FAILURE);
                }
            }
        }
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
        processes[i].stopped = 1;
    }
}

void run_rr_io(Process processes[], int count, int quantum_ms) {
    int finished_count = 0;
    double scheduler_start = get_wtime();

    start_all_stopped(processes, count);

    /*
     * Round Robin skips processes that are temporarily waiting for
     * simulated I/O and continues scheduling the remaining ready processes.
     */
    while (finished_count < count) {
        int made_progress = 0;

        for (int i = 0; i < count; i++) {
            if (processes[i].finished) {
                continue;
            }

            if (processes[i].in_io) {
                continue;
            }

            made_progress = 1;

            printf("running %s with PID %d\n", processes[i].command, processes[i].pid);

            if (kill(processes[i].pid, SIGCONT) == -1) {
                if (errno == ESRCH) {
                    processes[i].finish_time = get_wtime();
                    processes[i].finished = 1;
                    finished_count++;
                    continue;
                }

                perror("kill SIGCONT");
                exit(EXIT_FAILURE);
            }

            processes[i].stopped = 0;

            usleep((useconds_t)quantum_ms * 1000);

            int status;
            pid_t result = waitpid(processes[i].pid, &status, WNOHANG | WUNTRACED);

            if (result == processes[i].pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    processes[i].finish_time = get_wtime();
                    processes[i].finished = 1;
                    finished_count++;

                    print_process_finished(&processes[i], scheduler_start);
                } else if (WIFSTOPPED(status)) {
                    processes[i].stopped = 1;
                    processes[i].io_completed = 0;
                }
            } else if (result == 0) {
                if (processes[i].in_io) {
                    printf("PID %d entered I/O. Scheduler will run other processes.\n", processes[i].pid);
                    continue;
                }

                /*
                 * If the process is still running after its quantum, it is
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
                    processes[i].stopped = 1;
                }
            } else {
                if (errno == EINTR) {
                    continue;
                }

                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }

        /*
         * If all unfinished processes are waiting for I/O, avoid busy-waiting.
         */
        if (!made_progress) {
            usleep(10000);
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

    setup_io_signal_handlers();

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

        global_processes = processes;
        global_process_count = process_count;

        run_fcfs_io(processes, process_count);
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

        global_processes = processes;
        global_process_count = process_count;

        run_rr_io(processes, process_count, quantum_ms);
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}