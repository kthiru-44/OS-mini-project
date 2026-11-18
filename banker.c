/* Banker's safety algorithm. Need = Max - Allocation. */
#include "project.h"
#include <stdio.h>

#define MAXP 10
#define MAXR 10

typedef struct {
  int p, r;
  int avail[MAXR], alloc[MAXP][MAXR], max[MAXP][MAXR];
} State;

static int check(const State *s) {
  int need[MAXP][MAXR], work[MAXR], finish[MAXP] = {0}, seq[MAXP], n = 0;

  printf("      Alloc / Max / Need\n");
  for (int i = 0; i < s->p; i++) {
    printf("P%d   ", i);
    for (int j = 0; j < s->r; j++)
      need[i][j] = s->max[i][j] - s->alloc[i][j];
    for (int j = 0; j < s->r; j++) printf("%d ", s->alloc[i][j]);
    printf("| ");
    for (int j = 0; j < s->r; j++) printf("%d ", s->max[i][j]);
    printf("| ");
    for (int j = 0; j < s->r; j++) printf("%d ", need[i][j]);
    printf("\n");
  }
  printf("Available:");
  for (int j = 0; j < s->r; j++) {
    printf(" %d", s->avail[j]);
    work[j] = s->avail[j];
  }
  printf("\n");

  /* keep looking for a process whose Need fits in Work */
  int progress = 1;
  while (progress) {
    progress = 0;
    for (int i = 0; i < s->p; i++) {
      if (finish[i])
        continue;
      int ok = 1;
      for (int j = 0; j < s->r; j++)
        if (need[i][j] > work[j])
          ok = 0;
      if (ok) {
        for (int j = 0; j < s->r; j++)
          work[j] += s->alloc[i][j]; /* it finishes and releases everything */
        finish[i] = 1;
        seq[n++] = i;
        progress = 1;
      }
    }
  }

  if (n == s->p) {
    printf("SAFE state. Safe sequence:");
    for (int i = 0; i < n; i++)
      printf(" P%d%s", seq[i], i < n - 1 ? " ->" : "");
    printf("\n");
    return 1;
  }
  printf("UNSAFE state: no process can be guaranteed to finish.\n");
  return 0;
}

/* Reads one number; prints a prompt only when typing at the keyboard. */
static int get(FILE *in, int prompt, const char *what, int lo, int hi, int *v) {
  if (prompt)
    printf("%s: ", what);
  if (fscanf(in, "%d", v) != 1 || *v < lo || *v > hi) {
    fprintf(stderr, "Error: %s must be a number from %d to %d\n", what, lo, hi);
    return 1;
  }
  return 0;
}

/* File format: P R, then Available (R numbers), Allocation (P rows),
 * Maximum (P rows). */
static int read_state(FILE *in, int prompt, State *s) {
  char name[64];
  if (get(in, prompt, "Number of processes", 1, MAXP, &s->p) ||
      get(in, prompt, "Number of resource types", 1, MAXR, &s->r))
    return 1;
  for (int j = 0; j < s->r; j++) {
    snprintf(name, sizeof(name), "Available[R%d]", j);
    if (get(in, prompt, name, 0, 1000, &s->avail[j]))
      return 1;
  }
  for (int i = 0; i < s->p; i++)
    for (int j = 0; j < s->r; j++) {
      snprintf(name, sizeof(name), "Allocation[P%d][R%d]", i, j);
      if (get(in, prompt, name, 0, 1000, &s->alloc[i][j]))
        return 1;
    }
  for (int i = 0; i < s->p; i++)
    for (int j = 0; j < s->r; j++) {
      snprintf(name, sizeof(name), "Maximum[P%d][R%d]", i, j);
      if (get(in, prompt, name, 0, 1000, &s->max[i][j]))
        return 1;
      if (s->max[i][j] < s->alloc[i][j]) {
        fprintf(stderr, "Error: Maximum cannot be less than Allocation\n");
        return 1;
      }
    }
  return 0;
}

/* Custom state from --banker-file FILE, or typed in if no file is given. */
int run_banker(void) {
  State s;
  FILE *f = stdin;
  if (cfg.banker_file && !(f = fopen(cfg.banker_file, "r"))) {
    perror(cfg.banker_file);
    return 1;
  }
  int rc = read_state(f, f == stdin, &s);
  if (f != stdin)
    fclose(f);
  if (rc)
    return 1;
  printf("\n=== Banker's algorithm: custom state ===\n");
  check(&s);
  return 0;
}

static const State BASE = {5, 3, {3, 3, 2},
                           {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}},
                           {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}}};

int run_banker_safe(void) {
  printf("\n=== Banker's algorithm: safe case ===\n");
  return check(&BASE) ? 0 : 1;
}

int run_banker_unsafe(void) {
  State s = BASE;
  s.avail[0] = s.avail[1] = s.avail[2] = 0; /* nothing free -> nobody can finish */
  printf("\n=== Banker's algorithm: unsafe case ===\n");
  return check(&s) ? 1 : 0; /* success = detected unsafe */
}
