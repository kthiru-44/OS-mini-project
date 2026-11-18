/* Producer-consumer logger: bounded queue, mutex + condition variables. */
#include "project.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define QSIZE 3
#define MAX_PRODUCERS 10
#define LOG_FILE "campuscompute.log"

static char queue[QSIZE][64];
static int in, out, count, done;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

static void *producer(void *arg) {
  int id = *(int *)arg;
  for (int e = 1; e <= cfg.events; e++) {
    pthread_mutex_lock(&lock);
    while (count == QSIZE) /* queue full: wait */
      pthread_cond_wait(&not_full, &lock);
    snprintf(queue[in], sizeof(queue[in]), "producer %d: event %d", id, e);
    printf("[producer %d] queued event %d (queue size %d)\n", id, e, count + 1);
    in = (in + 1) % QSIZE;
    count++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

static void *consumer(void *arg) {
  FILE *log = arg;
  while (1) {
    pthread_mutex_lock(&lock);
    while (count == 0 && !done) /* queue empty: wait */
      pthread_cond_wait(&not_empty, &lock);
    if (count == 0 && done) { /* nothing left and producers finished */
      pthread_mutex_unlock(&lock);
      break;
    }
    char msg[64];
    strcpy(msg, queue[out]);
    out = (out + 1) % QSIZE;
    count--;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&lock);

    fprintf(log, "%s\n", msg);
    printf("[consumer] wrote \"%s\" to log\n", msg);
  }
  return NULL;
}

int run_logger(void) {
  pthread_t prod[MAX_PRODUCERS], cons;
  int ids[MAX_PRODUCERS];
  FILE *log = fopen(LOG_FILE, "w");
  if (!log) {
    perror("fopen");
    return 1;
  }
  in = out = count = done = 0;
  printf("\n=== Producer-consumer logger (%d producers x %d events, queue %d) "
         "===\n",
         cfg.producers, cfg.events, QSIZE);

  pthread_create(&cons, NULL, consumer, log);
  for (int i = 0; i < cfg.producers; i++) {
    ids[i] = i + 1;
    pthread_create(&prod[i], NULL, producer, &ids[i]);
  }
  for (int i = 0; i < cfg.producers; i++)
    pthread_join(prod[i], NULL);

  pthread_mutex_lock(&lock);
  done = 1; /* tell the consumer no more events are coming */
  pthread_cond_signal(&not_empty);
  pthread_mutex_unlock(&lock);
  pthread_join(cons, NULL);
  fclose(log);

  printf("All threads joined. %d events written to %s\n",
         cfg.producers * cfg.events, LOG_FILE);
  return 0;
}
