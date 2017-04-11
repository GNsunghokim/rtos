#include <string.h>
#include <util/queue.h>

Queue* queue_create(DataType type, PoolType pool, size_t size) {
	return (Queue*)collection_create(type, pool, size);
}

void queue_destroy(Queue* this) {
	void (*free)(void*) = this->free;
	free(this);
	this = NULL;
}

