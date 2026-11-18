/* IPC: POSIX shared memory + named semaphore between parent and child. */
#include "project.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_NAME "/campuscompute_shm"
#define SEM_NAME "/campuscompute_sem"

typedef struct {
  int job_id;
  int burst;
  char message[32];
} Packet;

int run_ipc(void) {
  printf("\n=== Shared memory + semaphore IPC ===\n");
  sem_unlink(SEM_NAME); /* remove leftovers from our own earlier run */
  shm_unlink(SHM_NAME);

  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    perror("shm_open");
    return 1;
  }
  ftruncate(fd, sizeof(Packet));
  Packet *p = mmap(NULL, sizeof(Packet), PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd, 0);
  sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0600, 0); /* starts at 0 */
  if (p == MAP_FAILED || sem == SEM_FAILED) {
    perror("mmap/sem_open");
    return 1;
  }

  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) { /* child = writer */
    p->job_id = cfg.job_id;
    p->burst = cfg.burst;
    strcpy(p->message, "job submitted");
    printf("[child %d] wrote job %d, burst %d into shared memory\n", getpid(),
           p->job_id, p->burst);
    sem_post(sem); /* signal: data is ready */
    fflush(stdout);
    _exit(0);
  }

  sem_wait(sem); /* parent = reader, blocks until the child posts */
  printf("[parent %d] semaphore released, read: job %d, burst %d, \"%s\"\n",
         getpid(), p->job_id, p->burst, p->message);
  waitpid(pid, NULL, 0);

  /* clean up only the objects this program created */
  munmap(p, sizeof(Packet));
  close(fd);
  sem_close(sem);
  sem_unlink(SEM_NAME);
  shm_unlink(SHM_NAME);
  printf("Shared memory and semaphore removed.\n");
  return 0;
}

/* Used by --cleanup: removes our objects (if any) and checks they are gone. */
int run_cleanup(void) {
  int fd, bad = 0;
  sem_unlink(SEM_NAME);
  shm_unlink(SHM_NAME);

  printf("\n=== Cleanup verification ===\n");
  fd = shm_open(SHM_NAME, O_RDONLY, 0);
  printf("Shared memory %s: %s\n", SHM_NAME, fd < 0 ? "removed" : "STILL EXISTS");
  if (fd >= 0) {
    close(fd);
    bad = 1;
  }
  sem_t *s = sem_open(SEM_NAME, 0);
  printf("Semaphore %s: %s\n", SEM_NAME, s == SEM_FAILED ? "removed" : "STILL EXISTS");
  if (s != SEM_FAILED) {
    sem_close(s);
    bad = 1;
  }
  int stray = (waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD);
  printf("Child processes left: %s\n", stray ? "none" : "SOME");
  return bad || !stray;
}
