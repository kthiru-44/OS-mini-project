/* CPU scheduling: FCFS, Round Robin, non-preemptive Priority.
 * Tie-break rule: earlier arrival first, then smaller job ID. */
#include "project.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS 20

typedef struct {
  int id, arrival, burst, priority; /* lower priority number = more urgent */
  int remaining, completion;
  char status[12]; /* READY -> COMPLETED */
} Job;

static int n; /* number of jobs */

static void add_job(Job j[], int arrival, int burst, int priority) {
  Job *p = &j[n];
  p->id = n + 1;
  p->arrival = arrival;
  p->burst = burst;
  p->priority = priority;
  p->remaining = burst;
  p->completion = 0;
  strcpy(p->status, "READY");
  n++;
}

static void default_jobs(Job j[]) {
  add_job(j, 0, 5, 2);
  add_job(j, 1, 3, 1);
  add_job(j, 2, 8, 4);
  add_job(j, 3, 6, 2);
  add_job(j, 4, 4, 2);
}

/* Reads lines of "arrival burst priority" and validates each one.
 * A blank line ends the input; '#' lines are comments. Returns 0 if valid. */
static int read_jobs(FILE *in, Job j[], int prompt) {
  char line[128];
  int lineno = 0;
  if (prompt)
    printf("Enter jobs, one per line: arrival burst priority (blank line to "
           "finish)\n");
  while (1) {
    if (prompt)
      printf("job %d> ", n + 1);
    if (!fgets(line, sizeof(line), in))
      break;
    lineno++;
    if (line[0] == '#')
      continue;
    if (line[0] == '\n') {
      if (prompt)
        break;
      continue;
    }
    int a, b, p;
    char extra;
    if (sscanf(line, "%d %d %d %c", &a, &b, &p, &extra) != 3 || a < 0 ||
        a > 1000 || b < 1 || b > 100 || p < 1 || p > 10) {
      fprintf(stderr,
              "Error (line %d): expected 'arrival(0-1000) burst(1-100) "
              "priority(1-10)'\n",
              lineno);
      return 1;
    }
    if (n == MAX_JOBS) {
      fprintf(stderr, "Error: at most %d jobs\n", MAX_JOBS);
      return 1;
    }
    add_job(j, a, b, p);
  }
  if (n == 0) {
    fprintf(stderr, "Error: no jobs entered\n");
    return 1;
  }
  return 0;
}

/* Sort by arrival (stable) so the algorithms can assume arrival order,
 * then renumber IDs J1, J2, ... in that order. */
static void sort_jobs(Job j[]) {
  for (int i = 1; i < n; i++) {
    Job key = j[i];
    int k = i - 1;
    while (k >= 0 && j[k].arrival > key.arrival) {
      j[k + 1] = j[k];
      k--;
    }
    j[k + 1] = key;
  }
}

/* Chooses the job source: file, typed input, or the default set. */
static int get_jobs(Job j[]) {
  int rc = 0;
  n = 0;
  if (cfg.jobs_file) {
    FILE *f = fopen(cfg.jobs_file, "r");
    if (!f) {
      perror(cfg.jobs_file);
      return 1;
    }
    rc = read_jobs(f, j, 0);
    fclose(f);
  } else if (cfg.input_jobs) {
    rc = read_jobs(stdin, j, 1);
  } else {
    default_jobs(j);
  }
  if (rc == 0)
    sort_jobs(j);
  return rc;
}

static void report(Job j[], int show, double *avg_wt, double *avg_tat) {
  double wt_sum = 0, tat_sum = 0;
  if (show)
    printf("\nJob  Arr  Burst  Pri  Completion  Turnaround  Waiting  Status\n");
  for (int i = 0; i < n; i++) {
    int tat = j[i].completion - j[i].arrival;
    int wt = tat - j[i].burst;
    tat_sum += tat;
    wt_sum += wt;
    if (show)
      printf("J%-3d %-4d %-6d %-4d %-11d %-11d %-8d %s\n", j[i].id,
             j[i].arrival, j[i].burst, j[i].priority, j[i].completion, tat, wt,
             j[i].status);
  }
  *avg_wt = wt_sum / n;
  *avg_tat = tat_sum / n;
  if (show)
    printf("Average turnaround = %.2f, average waiting = %.2f\n", *avg_tat,
           *avg_wt);
}

static void finish(Job *p, int t) {
  p->completion = t;
  strcpy(p->status, "COMPLETED");
}

static void fcfs(Job j[], int show) {
  int t = 0;
  if (show)
    printf("Timeline:");
  for (int i = 0; i < n; i++) {
    if (t < j[i].arrival)
      t = j[i].arrival; /* CPU idle until the job arrives */
    if (show)
      printf(" [%d-%d J%d]", t, t + j[i].burst, j[i].id);
    t += j[i].burst;
    finish(&j[i], t);
  }
  if (show)
    printf("\n");
}

static void round_robin(Job j[], int show) {
  int queue[4096], head = 0, tail = 0, next = 0, t = 0, done = 0;
  if (show)
    printf("Timeline:");
  while (done < n) {
    while (next < n && j[next].arrival <= t) /* admit arrivals */
      queue[tail++ % 4096] = next++;
    if (head == tail) { /* queue empty: jump to the next arrival */
      t = j[next].arrival;
      continue;
    }
    int i = queue[head++ % 4096];
    int run = j[i].remaining < cfg.quantum ? j[i].remaining : cfg.quantum;
    if (show)
      printf(" [%d-%d J%d]", t, t + run, j[i].id);
    t += run;
    j[i].remaining -= run;
    /* jobs that arrived during this slice go in before the preempted job */
    while (next < n && j[next].arrival <= t)
      queue[tail++ % 4096] = next++;
    if (j[i].remaining > 0)
      queue[tail++ % 4096] = i;
    else {
      finish(&j[i], t);
      done++;
    }
  }
  if (show)
    printf("\n");
}

static void priority(Job j[], int show) {
  int t = 0, done = 0, finished[MAX_JOBS] = {0};
  if (show)
    printf("Timeline:");
  while (done < n) {
    int best = -1;
    for (int i = 0; i < n; i++) {
      if (finished[i] || j[i].arrival > t)
        continue;
      /* jobs are in arrival/ID order, so '<' keeps the tie-break rule */
      if (best == -1 || j[i].priority < j[best].priority)
        best = i;
    }
    if (best == -1) { /* nothing has arrived yet */
      t++;
      continue;
    }
    if (show)
      printf(" [%d-%d J%d]", t, t + j[best].burst, j[best].id);
    t += j[best].burst;
    finish(&j[best], t);
    finished[best] = 1;
    done++;
  }
  if (show)
    printf("\n");
}

static int run(const char *name, void (*algo)(Job[], int)) {
  Job j[MAX_JOBS];
  double wt, tat;
  if (get_jobs(j))
    return 1;
  printf("\n=== %s ===\n", name);
  algo(j, 1);
  report(j, 1, &wt, &tat);
  return 0;
}

int run_fcfs(void) { return run("FCFS", fcfs); }

int run_rr(void) {
  printf("\nTime quantum = %d\n", cfg.quantum);
  return run("Round Robin", round_robin);
}

int run_priority(void) {
  printf("\nTie-break: same priority -> earlier arrival, then smaller ID\n");
  return run("Priority (non-preemptive)", priority);
}

int run_compare(void) {
  const char *names[3] = {"FCFS", "Round Robin", "Priority"};
  void (*algos[3])(Job[], int) = {fcfs, round_robin, priority};
  Job base[MAX_JOBS];
  if (get_jobs(base))
    return 1;
  printf("\n=== Comparison (same %d jobs, quantum %d) ===\n", n, cfg.quantum);
  printf("%-14s %-16s %s\n", "Policy", "Avg turnaround", "Avg waiting");
  for (int k = 0; k < 3; k++) {
    Job j[MAX_JOBS];
    double wt, tat;
    memcpy(j, base, sizeof(j)); /* every policy starts from the same jobs */
    algos[k](j, 0);
    report(j, 0, &wt, &tat);
    printf("%-14s %-16.2f %.2f\n", names[k], tat, wt);
  }
  return 0;
}
