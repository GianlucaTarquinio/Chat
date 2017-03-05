#ifndef PQUEUE_H
#define PQUEUE_H

#include <pthread.h>

typedef struct pqueue {
	int size;
	int maxSize;
	void** head;
	void** tail; //Pointer to next open space
	int (*compare)(const void*, const void*);
	void** base;
	pthread_mutex_t lock;
} PQueue;

PQueue* initQueue(int size, int (*compare)(const void*, const void*));
int push(PQueue* q, const void* node);
void* pop(PQueue* q);
void* peek(PQueue* q);
int isEmpty(PQueue* q);
void freeQueue(PQueue* q);

#endif
