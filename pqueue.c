#include <stdlib.h>
#include <errno.h>
#include "pqueue.h"

/** Sets the fields of a priority queue to the appropriate values.
* Important: Both the new PQueue and the base field of the new PQueue
*            are dynamically allocated, and must be freed manually
*            (can be done using freeQueue())
*
* @param q The queue to initialize
* @param size The number of elements the queue should be able to hold
* @param compare Pointer to function to compare priotiry of nodes.
*                Should return an int less than, equal to, or greater
*                than 0 if the node pointed to by the first argument
*                has lower, equal, or higher priority than the node
*                pointed to by the second argument, respectively.
* @return Pointer to new dynamically allocated PQueue with appropriate initial values if it is successful.
*         If it is unsuccessful, it returns NULL, and sets the global variable errno to indicate the error.
*         Error codes: 1 - Could not dynamically allocate memory
*                      2 - Invalid size
*/
PQueue* initQueue(int size, int (*compare)(const void*, const void*)) {
	if(size < 1) {
		errno = 2;
		return NULL;
	}
	PQueue* q = (PQueue*) malloc(sizeof(PQueue));
	if(q == NULL) {
		errno = 1;
		return NULL;
	}
	q->size = 0;
	q->maxSize = size;
	q->compare = compare;
	q->base = (void**) calloc(size, sizeof(void*));
	if(q->base == NULL) {
		free(q);
		errno = 1;
		return NULL;
	}
	q->head = q->base;
	pthread_mutex_init(&(q->lock), NULL);
	return q;
}

/** Returns a pointer to the space after the given space in the given priority queue
* (Space refers to a dynamically allocated memory location)
*
* @param q Queue to use
* @param space Space to find the next space of
* @return A pointer to the next space in the queue, or NULL if space was outside of the queue
*/
void** nextSpace(PQueue* q, void** space) {
	if(space < q->base || space >= q->base + q->maxSize) {
		return NULL;
	}
	if(space + 1 >= q->base + q->maxSize) {
		return q->base;
	}
	return space + 1;
}

/** Push an element into a priority queue, priority comparrison is done by the compare function
* pointed to in the PQueue.  That function should return an integer less than, equal to, or
* greater than 0, if the the node pointed to by the first argument has lower, equal, or
* higher priority than the node pointed to by the second argument, respectively.
*
* @param q The queue to push into
* @param node The element to push into the queue
* @return A non-negative integer that is the number of spaces remaining in the queue,
*         or -1 if the queue was full so the push did not happen.
*/
int push(PQueue* q, void* node) {
	pthread_mutex_lock(&(q->lock));
	if(q->size >= q->maxSize) {
		q->size = q->maxSize;
		pthread_mutex_unlock(&(q->lock));
		return -1;
	}
	void **current = q->head;
	void **next = nextSpace(q, q->head);
	int i = 0;
	while(next != NULL && i < q->size && (q->compare)(node, *next) <= 0) {
		current = next;
		next = nextSpace(q, current);
		i++;
	}
	*current = node;
	q->size++;
	pthread_mutex_unlock(&(q->lock));
	return q->maxSize - q->size;
}

/** Pop an element off of a priority queue
*
* @param q The queue to pop off of
* @return The element that was popped off of the queue, or NULL (with errno)
*					Error codes: 0 - Queue was empty
*											 1 - The head was outside of the queue
*/
void* pop(PQueue* q) {
	pthread_mutex_lock(&(q->lock));
	if(q->size <= 0) {
		q->size = 0;
		errno = 0;
		pthread_mutex_unlock(&(q->lock));
		return NULL;
	}
	void *popped = *(q->head);
	void **next = nextSpace(q, q->head);
	if(next == NULL) {
		errno = 1;
		pthread_mutex_unlock(&(q->lock));
		return NULL;
	}
	q->head = next;
	q->size--;
	pthread_mutex_unlock(&(q->lock));
	return popped;
}

/** Look at the head element of a queue
*
* @param q The queue to peek into
* @return The node at the head of the queue, or NULL if the queue is empty
*/
void* peek(PQueue* q) {
	pthread_mutex_lock(&(q->lock));
	if(q->size <= 0) {
		pthread_mutex_unlock(&(q->lock));
		return NULL;
	}
	pthread_mutex_unlock(&(q->lock));
	return *(q->head);
}

/** Checks if the queue is empty
*
* @param q The queue to check
* @return 0 if the queue is not empty, 1 if the queue is empty
*/
int isEmpty(PQueue* q) {
	return q->size == 0;
}

/** Free's all dynamically allocated memory associated with a priority queue
* Important: freeQueue() does not attempt to free the nodes themselves, as
*            they are not necessarily dnamically allocated.
*
* @param q Queue to free
*/
void freeQueue(PQueue* q) {
	free(q->base);
	free(q);
	return;
}
