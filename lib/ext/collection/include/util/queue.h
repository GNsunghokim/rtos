#ifndef __UTIL_QUEUE_H__
#define __UTIL_QUEUE_H__

#include <util/collection.h>

typedef struct _Queue Queue;

typedef struct _QueOps {
	bool	(*enqueue)(void* this, void* element);
	void*	(*dequeue)(void* this);
	void*	(*get)(void* this, int index);
	void*	(*peek)(void* this);
} QueueOps;

typedef struct _Queue {
	Collection;

	QueueOps;
} Queue;

Queue* queue_create(DataType type, PoolType pool, size_t size);
void queue_destroy(Queue* this);

#endif /* __UTIL_QUEUE_H__ */
