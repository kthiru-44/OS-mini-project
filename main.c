#include "project.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config cfg = {NULL, 0, 2, 3, 4, 101, 5, 3, 1, NULL};

typedef struct {
  const char *flag;
  const char *help;
  int (*run)(void);
  int in_demo; /* part of --demo? */
} Command;

static int run_demo(void);

static const Command commands[] = {
    {"--fcfs", "FCFS scheduling", run_fcfs, 1},
    {"--rr", "Round Robin scheduling", run_rr, 1},
    {"--priority", "Priority scheduling (non-preemptive)", run_priority, 1},
    {"--compare", "Compare all scheduling policies", run_compare, 1},
    {"--logger", "Producer-consumer logger (threads)", run_logger, 1},
    {"--ipc", "Shared memory + semaphore IPC", run_ipc, 1},
    {"--banker-safe", "Banker's algorithm: safe state", run_banker_safe, 1},
    {"--banker-unsafe", "Banker's algorithm: unsafe state", run_banker_unsafe,
     1},
    {"--worker-ok", "Worker process: normal exit", run_worker_ok, 1},
    {"--worker-fail", "Worker process: simulated failure", run_worker_fail, 1},
    {"--rlimit", "Resource limit demo (RLIMIT_CPU)", run_rlimit, 1},
    {"--banker", "Banker's algorithm: custom state (--banker-file FILE, or typed)",
     run_banker, 0},
    {"--cleanup", "Verify IPC objects and children are cleaned up", run_cleanup,
     0},
    {"--demo", "Run everything and print a summary", run_demo, 0},
};
#define NCMD ((int)(sizeof(commands) / sizeof(commands[0])))

/* Options that carry a number: --name N */
static const struct {
  const char *flag;
  int *value;
  int lo, hi;
  const char *help;
} num_opts[] = {
    {"--quantum", &cfg.quantum, 1, 100, "Round Robin time quantum (default 2)"},
    {"--producers", &cfg.producers, 1, 10, "logger producer threads (default 3)"},
    {"--events", &cfg.events, 1, 20, "events per producer (default 4)"},
    {"--job-id", &cfg.job_id, 1, 9999, "job ID sent over IPC (default 101)"},
    {"--burst", &cfg.burst, 1, 100, "burst sent over IPC (default 5)"},
    {"--exit-code", &cfg.exit_code, 1, 255, "simulated failure code (default 3)"},
    {"--cpu-limit", &cfg.cpu_limit, 1, 10, "RLIMIT_CPU seconds (default 1)"},
};
#define NNUM ((int)(sizeof(num_opts) / sizeof(num_opts[0])))

static void usage(const char *prog) {
  printf("Usage: %s [options] <command>\n\nCommands:\n", prog);
  for (int i = 0; i < NCMD; i++)
    printf("  %-16s %s\n", commands[i].flag, commands[i].help);
  printf("  %-16s %s\n", "--help", "Show this message");
  printf("\nOptions (put them before the command):\n");
  printf("  %-16s %s\n", "--jobs FILE", "read jobs from a file (arrival burst priority)");
  printf("  %-16s %s\n", "--input", "type the jobs in at the prompt");
  printf("  %-16s %s\n", "--banker-file F", "Banker state from a file");
  for (int i = 0; i < NNUM; i++)
    printf("  %-16s %s\n", num_opts[i].flag, num_opts[i].help);
  printf("\nExample: %s --jobs samples/jobs.txt --quantum 3 --compare\n", prog);
}

static int run_demo(void) {
  int result[NCMD] = {0}, failed = 0;
  for (int i = 0; i < NCMD; i++)
    if (commands[i].in_demo)
      result[i] = commands[i].run();
  int cleanup = run_cleanup();

  printf("\n=== Summary ===\n");
  for (int i = 0; i < NCMD; i++)
    if (commands[i].in_demo) {
      printf("  %-16s %s\n", commands[i].flag, result[i] ? "FAIL" : "PASS");
      failed += result[i] != 0;
    }
  printf("  %-16s %s\n", "--cleanup", cleanup ? "FAIL" : "PASS");
  failed += cleanup != 0;
  printf("%d check(s) failed.\n", failed);
  return failed != 0;
}

static int bad_arg(const char *prog, const char *what, const char *arg) {
  fprintf(stderr, "Error: %s '%s'\n", what, arg);
  usage(prog);
  return 1;
}

int main(int argc, char *argv[]) {
  const Command *cmd = NULL;

  for (int a = 1; a < argc; a++) {
    const char *arg = argv[a];
    int matched = 0;

    if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
      usage(argv[0]);
      return 0;
    }
    if (!strcmp(arg, "--input")) {
      cfg.input_jobs = 1;
      continue;
    }
    if (!strcmp(arg, "--jobs") || !strcmp(arg, "--banker-file")) {
      if (a + 1 >= argc)
        return bad_arg(argv[0], "missing value for", arg);
      if (!strcmp(arg, "--jobs"))
        cfg.jobs_file = argv[++a];
      else
        cfg.banker_file = argv[++a];
      continue;
    }
    for (int i = 0; i < NNUM && !matched; i++)
      if (!strcmp(arg, num_opts[i].flag)) {
        char *end;
        long v = a + 1 < argc ? strtol(argv[a + 1], &end, 10) : 0;
        if (a + 1 >= argc || *end != '\0' || v < num_opts[i].lo ||
            v > num_opts[i].hi) {
          fprintf(stderr, "Error: %s needs a number from %d to %d\n", arg,
                  num_opts[i].lo, num_opts[i].hi);
          return 1;
        }
        *num_opts[i].value = (int)v;
        a++;
        matched = 1;
      }
    if (matched)
      continue;
    for (int i = 0; i < NCMD && !matched; i++)
      if (!strcmp(arg, commands[i].flag)) {
        if (cmd)
          return bad_arg(argv[0], "only one command allowed, got extra", arg);
        cmd = &commands[i];
        matched = 1;
      }
    if (!matched)
      return bad_arg(argv[0], "invalid command or option", arg);
  }

  if (!cmd) {
    fprintf(stderr, "Error: no command given\n");
    usage(argv[0]);
    return 1;
  }
  return cmd->run();
}
