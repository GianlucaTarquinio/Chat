#include <stdlib.h>
#include <errno.h>
#include "pqueue.h"

int pqPush(PQueue **queue, void *data, int (*compare)(const void*, const void*), pthread_mutex_t lock) {
	return 1;
}

void *pqPop(PQueue **queue, pthread_mutex_t lock) {
	return NULL;
}

void freeQueue(PQueue **queue, pthread_mutex_t lock) {
	return;
}
