#ifndef __UTIL_DEQUE_H__
#define __UTIL_DEQUE_H__

#include <util/queue.h>

typedef struct _Deque Deque;

typedef struct _DequeOps {
	bool	(*add_first)(void* this, void*);
	bool	(*add_last)(void* this, void*);
	void*	(*remove_first)(void* this);
	void*	(*remove_last)(void* this);
	void*	(*get_first)(void* this);
	void*	(*get_last)(void* this);
} DequeOps;

typedef struct _Deque {
	Queue;

	DequeOps;
} Deque;

#endif /* __UTIL_DEQUE_H__ */
