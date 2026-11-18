/* Worker processes: fork(), waitpid() status checks, and setrlimit(). */
#include "project.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

/* Waits for the child and reports how it ended. Returns the wait status. */
static int wait_and_report(pid_t pid) {
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status))
    printf("[parent] child %d exited normally, exit code %d\n", pid,
           WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    printf("[parent] child %d killed by signal %d (%s)\n", pid,
           WTERMSIG(status), strsignal(WTERMSIG(status)));
  return status;
}

int run_worker_ok(void) {
  printf("\n=== Worker: normal completion ===\n");
  fflush(stdout);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    printf("[worker %d] running job... done\n", getpid());
    fflush(stdout);
    exit(0);
  }
  int s = wait_and_report(pid);
  return (WIFEXITED(s) && WEXITSTATUS(s) == 0) ? 0 : 1;
}

int run_worker_fail(void) {
  printf("\n=== Worker: SIMULATED failure  ===\n");
  fflush(stdout);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    printf("[worker %d] simulating a failure, exiting with code %d\n", getpid(),
           cfg.exit_code);
    fflush(stdout);
    exit(cfg.exit_code);
  }
  int s = wait_and_report(pid);
  if (WIFEXITED(s) && WEXITSTATUS(s) != 0)
    printf("[parent] failure detected (simulated)\n");
  return (WIFEXITED(s) && WEXITSTATUS(s) == cfg.exit_code) ? 0 : 1;
}

/* Child limits its own CPU time to 1 second, then spins. The kernel sends
 * SIGXCPU when the limit is reached and the child is terminated. */
int run_rlimit(void) {
  printf("\n=== Resource limit: RLIMIT_CPU = %d second(s) ===\n",
         cfg.cpu_limit);
  fflush(stdout);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    struct rlimit rl = {cfg.cpu_limit,
                        cfg.cpu_limit + 2}; /* soft: SIGXCPU, hard: SIGKILL */
    if (setrlimit(RLIMIT_CPU, &rl) < 0) {
      perror("setrlimit");
      exit(1);
    }
    printf("[child %d] CPU limit set to %ds, now busy-looping\n", getpid(),
           cfg.cpu_limit);
    fflush(stdout);
    while (1)
      ;
  }
  int s = wait_and_report(pid);
  if (WIFSIGNALED(s) && WTERMSIG(s) == SIGXCPU) {
    printf("Effect: kernel stopped the child when it used up its CPU time.\n");
    return 0;
  }
  return 1;
}
