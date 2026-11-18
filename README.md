# CampusCompute

Prerequisites: Linux/WSL Ubuntu, `gcc`, `make`.

    make                       # build ./campuscompute
    ./campuscompute --help     # commands and options
    ./campuscompute --demo     # run every feature + PASS/FAIL summary
    ./campuscompute --cleanup  # verify no IPC objects / children remain
    make clean                 # remove build files and log
    make zip SRN=<your SRN>    # create the submission ZIP

Syntax: `./campuscompute [options] <command>`

## Commands
| Command | Feature |
|---|---|
| `--fcfs` `--rr` `--priority` `--compare` | CPU scheduling. Priority tie-break: earlier arrival, then smaller ID |
| `--logger` | Producer-consumer logger (pthreads, mutex + condvars) -> `campuscompute.log` |
| `--ipc` | POSIX shared memory + named semaphore (parent/child) |
| `--banker-safe` `--banker-unsafe` | Banker's safety algorithm, built-in test states |
| `--banker` | Banker's algorithm on your own state (`--banker-file FILE` or typed in) |
| `--worker-ok` `--worker-fail` | fork/waitpid; the failure is **simulated** |
| `--rlimit` | `setrlimit(RLIMIT_CPU)`: child is killed by SIGXCPU |
| `--demo` `--cleanup` | integrated run / cleanup check |

## Options (before the command)
| Option | Meaning |
|---|---|
| `--jobs FILE` | read jobs from a file, one per line: `arrival burst priority` (`#` = comment) |
| `--input` | type the jobs in at the prompt (blank line to finish) |
| `--quantum N` | Round Robin time quantum (default 2) |
| `--producers N` `--events N` | logger threads / events per thread (default 3 / 4) |
| `--job-id N` `--burst N` | packet sent over shared memory (default 101 / 5) |
| `--exit-code N` | exit code of the simulated failing worker (default 3) |
| `--cpu-limit N` | RLIMIT_CPU seconds (default 1) |
| `--banker-file FILE` | Banker state: `P R`, Available, Allocation rows, Maximum rows |

Without `--jobs`/`--input`, a fixed set of 5 jobs is used. Invalid jobs or numbers print an error and exit 1.

Examples:

    ./campuscompute --jobs samples/jobs.txt --quantum 3 --compare
    ./campuscompute --input --priority
    ./campuscompute --banker-file samples/banker_unsafe.txt --banker

IPC names used: `/campuscompute_shm`, `/campuscompute_sem` (only these are removed).
