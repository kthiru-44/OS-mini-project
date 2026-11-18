#ifndef PROJECT_H
#define PROJECT_H

/* Settings chosen on the command line (defaults are set in main.c). */
typedef struct {
  const char *jobs_file;   /* --jobs FILE   : read jobs from a file          */
  int input_jobs;          /* --input       : type the jobs in               */
  int quantum;             /* --quantum N   : Round Robin time quantum       */
  int producers, events;   /* --producers N --events N : logger workload     */
  int job_id, burst;       /* --job-id N --burst N     : IPC packet          */
  int exit_code;           /* --exit-code N : simulated worker failure code  */
  int cpu_limit;           /* --cpu-limit N : seconds for RLIMIT_CPU         */
  const char *banker_file; /* --banker-file FILE : Banker state from a file  */
} Config;
extern Config cfg;

/* Every feature returns 0 on success and non-zero on failure. */
int run_fcfs(void);
int run_rr(void);
int run_priority(void);
int run_compare(void);
int run_logger(void);
int run_ipc(void);
int run_banker_safe(void);
int run_banker_unsafe(void);
int run_banker(void); /* custom state: --banker-file FILE, or typed in */
int run_worker_ok(void);
int run_worker_fail(void);
int run_rlimit(void);
int run_cleanup(void);

#endif
