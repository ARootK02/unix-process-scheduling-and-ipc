# Unix Process Scheduling and IPC

## Overview

This repository contains an academic Operating Systems project focused on Unix/POSIX process management, inter-process communication, synchronization, and process scheduling.

The project includes Bash scripting, process-based Monte Carlo integration using shared memory, semaphore-protected critical sections, synchronization pseudocode, theoretical scheduling analysis, and a user-level process scheduler supporting FCFS and Round Robin policies. An extended scheduler also handles workloads that simulate I/O through Unix signals.

## Project Information

- **Course:** Operating Systems
- **Academic context:** Computer Engineering and Informatics Department, University of Patras
- **Project type:** University laboratory project
- **Year:** 2023–2024
- **Main technologies:** C, Bash, Unix/POSIX system calls

## Authors

- Antreas Kerkidis
- Sotiris Chatzigiannis
- Varnavas Nikolaou
- Charis Pissouros

## Project Objectives

The project was developed to demonstrate core Operating Systems concepts through practical implementation and theoretical analysis.

The main objectives were:

- Implement a Bash menu-based script for managing business records stored in a CSV file.
- Use multiple processes to approximate an integral with the Monte Carlo method.
- Apply `mmap` shared memory for communication between parent and child processes.
- Protect shared updates using semaphores.
- Describe synchronization logic for a shared-resource scenario using pseudocode.
- Analyze FCFS, SJF, SRTF, and Round Robin scheduling algorithms theoretically.
- Implement a user-level process scheduler using `fork`, `exec`, `waitpid`, `SIGSTOP`, and `SIGCONT`.
- Extend the scheduler to handle simulated I/O using `SIGUSR1`, `SIGUSR2`, `SIGSTOP`, and `SIGCONT`.

## Repository Structure

```text
unix-process-scheduling-and-ipc/
├── .gitignore
├── README.md
├── part-1/
│   ├── shell-scripting/
│   │   ├── business-manager.sh
│   │   └── Businesses.csv
│   ├── monte-carlo/
│   │   ├── integral_mc_shm.c
│   │   └── integral_mc_shm_sem.c
│   └── synchronization/
│       └── restaurant-synchronization-pseudocode.txt
├── part-2/
│   ├── scheduler/
│   │   ├── Makefile
│   │   ├── scheduler.c
│   │   ├── scheduler_io.c
│   │   ├── homogeneous.txt
│   │   ├── mixed.txt
│   │   ├── reverse.txt
│   └── workloads/
│       ├── Makefile
│       ├── work.c
│       └── work_io.c
└── docs/
    └── operating-systems-report-gr.pdf
```

## Repository Contents

### Part 1: Shell Scripting, IPC, and Synchronization

The first part includes:

- A Bash script for managing business records from a CSV file.
- Two Monte Carlo implementations using multiple processes and shared memory.
- A synchronization pseudocode solution for a shared-resource scenario.
- A theoretical scheduling analysis included in the project report.

### Part 2: User-Level Process Scheduling

The second part includes:

- CPU-bound workload programs generated from `work.c`.
- An I/O-simulating workload generated from `work_io.c`.
- A basic user-level scheduler implementing FCFS and Round Robin.
- An extended scheduler that handles simulated I/O through Unix signals.

## Build and Run Instructions

This project uses Unix/POSIX system calls and should be compiled and executed in a Unix-like environment, such as Linux, macOS, WSL, or MSYS2.

### 1. Run the Bash business manager

```bash
cd part-1/shell-scripting
bash business-manager.sh
```

### 2. Compile and run the Monte Carlo programs

```bash
cd part-1/monte-carlo

gcc -Wall -Wextra -O2 -o integral_mc_shm integral_mc_shm.c -lm
gcc -Wall -Wextra -O2 -o integral_mc_shm_sem integral_mc_shm_sem.c -lm -pthread

./integral_mc_shm 1000000
./integral_mc_shm_sem 1000000
```

### 3. Compile the workload programs

```bash
cd part-2/workloads
make
```

This generates the workload executables used by the schedulers.

### 4. Compile the schedulers

```bash
cd ../scheduler
make
```

This generates:

```text
scheduler
scheduler_io
```

### 5. Run the basic scheduler

```bash
./scheduler FCFS reverse.txt
./scheduler RR 1000 reverse.txt
```

### 6. Run the extended I/O-aware scheduler

```bash
./scheduler_io FCFS mixed.txt
./scheduler_io RR 1000 mixed.txt
```

### 7. Clean generated files

From the scheduler folder:

```bash
make clean
```

From the workloads folder:

```bash
cd ../workloads
make clean
```

Generated executables are intentionally excluded from the repository.

## Project Report

A Greek project report is included:

```text
docs/operating-systems-report-gr.pdf
```

The report explains the design, implementation, testing results, theoretical scheduling calculations, problems fixed, limitations, and conclusions.

## Important Notes

- The project is designed for Unix/POSIX-compatible environments.
- The workload program `work5x2_io` is not intended to be executed manually as a normal standalone program, because it intentionally stops itself with `SIGSTOP`.
- `work5x2_io` should be executed through `scheduler_io`, which resumes it with `SIGCONT`.
- Generated binaries and object files are not included in the repository.

## Languages and Tools

- C
- Bash
- Makefile
- Unix/POSIX system calls
- Shared memory with `mmap`
- Semaphores
- Unix signals

## License / Copyright

This project was developed as part of a university Operating Systems course.

The source code is provided for educational and portfolio purposes. The included dataset and assignment context belong to their respective owners.
