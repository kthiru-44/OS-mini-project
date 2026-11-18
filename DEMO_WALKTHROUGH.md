# CampusCompute - Demo Walkthrough

## Setup
```bash
export SRN='PES2UG25CS586'; export PS1='[$SRN] \w$ '
make clean && make
```

## Demo script (about 5 minutes)

| # | Command | Say |
|---|---|---|
| 1 | `./campuscompute --demo` | "Every module runs and the summary is real PASS/FAIL." |
| 2 | `./campuscompute --jobs samples/jobs.txt --compare` | "Jobs come from a file; results are computed, not hard-coded." |
| 3 | `./campuscompute --jobs samples/jobs.txt --quantum 3 --rr` | "Quantum is an input. Check one row on paper." |
| 4 | `./campuscompute --input --priority` | Let the teacher type their own jobs, e.g. `0 5 2`, `1 3 1`, `2 4 2`, blank line. |
| 5 | `./campuscompute --banker-file samples/banker_unsafe.txt --banker` | UNSAFE. Change Available in the file to `3 3 2`, rerun: SAFE + sequence. |
| 6 | `./campuscompute --producers 4 --events 3 --logger` then `cat campuscompute.log` | "Bounded queue of 3; mutex + condition variables stop overflow/underflow." |
| 7 | `./campuscompute --job-id 7 --burst 9 --ipc` | "Child writes into shared memory; the semaphore makes the parent wait." |
| 8 | `./campuscompute --worker-ok` / `--exit-code 9 --worker-fail` | "fork + waitpid; the parent reads the exit status. The failure is simulated." |
| 9 | `./campuscompute --cpu-limit 2 --rlimit` | "Kernel sends SIGXCPU when the CPU limit is hit." |
| 10 | `./campuscompute --cleanup; ls /dev/shm` | "No children, shared memory or semaphores left." |
| 11 | `./campuscompute --bogus` | "Bad input gives an error and the usage text." |

## Files
| File | Role |
|---|---|
| `main.c` | Command/option parsing, `--demo`, usage |
| `scheduler.c` | Job input + validation, FCFS, Round Robin, Priority, compare |
| `logger.c` | Bounded buffer, producer threads, consumer thread, log file |
| `ipc.c` | `shm_open`/`mmap`, named semaphore, `--cleanup` |
| `banker.c` | Need = Max - Alloc, safety loop, file/typed input |
| `worker.c` | `fork`/`waitpid`, simulated failure, `setrlimit` |
| `project.h` | Config struct and function list |
| `samples/` | `jobs.txt`, `banker_safe.txt`, `banker_unsafe.txt` |

## Likely questions
- **Hard-coded?** Only defaults. Jobs, quantum, thread counts, IPC packet and Banker state are inputs.
- **Why a semaphore starting at 0?** The parent blocks until the child posts, so it never reads half-written data.
- **Why `while` around `cond_wait`?** A thread can wake up when the condition is no longer true, so it re-checks.
- **Why is Round Robin worse on average here?** It favours response time, not turnaround.
