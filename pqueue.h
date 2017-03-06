#ifndef PQUEUE_H
#define PQUEUE_H

#include <pthread.h>

typedef struct pqueue {
	void *data;
	struct pqueue *next;
} PQueue;

int pqPush(PQueue **queue, void *data, int (*compare)(const void*, const void*), pthread_mutex_t lock);
void *pqPop(PQueue **queue, pthread_mutex_t lock);
void freeQueue(PQueue **queue, pthread_mutex_t lock);

#endif
