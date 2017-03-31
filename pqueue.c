#include <stdlib.h>
#include <errno.h>
#include "pqueue.h"

int pqPush(PQueue **queue, void *data, int (*compare)(const void*, const void*), pthread_mutex_t *lock) {
	if(lock) pthread_mutex_lock(lock);
	PQueue *node = (PQueue *) malloc(sizeof(PQueue));
	if(!node) {
		if(lock) pthread_mutex_unlock(lock);
		return 1;
	}
	node->data = data;
	PQueue *next = *queue;
	if(!next) {
		*queue = node;
		node->next = NULL;
		if(lock) pthread_mutex_unlock(lock);
		return 0;
	}
	PQueue *current = NULL;
	while(next && compare(data, next->data) >= 0) {
		current = next;
		next = next->next;
	}
	node->next = next;
	if(current) {
		current->next = node;
	} else {
		*queue = node;
	}
	if(lock) pthread_mutex_unlock(lock);
	return 0;
}

void *pqPop(PQueue **queue, pthread_mutex_t *lock) {
	if(lock) pthread_mutex_lock(lock);
	PQueue *head = *queue;
	void *popped;
	if(!head) {
		if(lock) pthread_mutex_unlock(lock);
		return NULL;
	}
	popped = head->data;
	*queue = head->next;
	free(head);
	if(lock) pthread_mutex_unlock(lock);
	return popped;
}

void freeQueue(PQueue **queue, pthread_mutex_t *lock) {
	if(lock) pthread_mutex_lock(lock);
	int i;
	PQueue *current = *queue, *temp;
	while(current != NULL) {
		temp = current;
		current = temp->next;
		free(temp);
	}
	*queue = NULL;
	if(lock) pthread_mutex_unlock(lock);
}
