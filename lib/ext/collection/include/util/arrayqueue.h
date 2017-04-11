#ifndef __UTIL_ARRAY_QUEUE_H__
#define __UTIL_ARRAY_QUEUE_H__

#include <util/queue.h>

typedef struct _ArrayQueue ArrayQueue;

typedef struct _ArrayQueueOps {
	bool	(*is_available)(ArrayQueue* this);
	bool	(*resize)(ArrayQueue* this, size_t capacity, void (*popped)(void*));
} ArrayQueueOps;

typedef struct _ArrayQueue {
	Queue;

	size_t	head;
	size_t	tail;
	size_t	capacity;
	void**	array;
	ArrayQueueOps;
} ArrayQueue;

ArrayQueue* arrayqueue_create(DataType type, PoolType pool, size_t initial_capacity);
void arrayqueue_destroy(ArrayQueue* this);

#endif /* __UTIL_ARRAY_QUEUE_H__ */
