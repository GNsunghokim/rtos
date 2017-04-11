#include <string.h>
#include <util/list.h>

List* list_create(DataType type, PoolType pool, size_t size) {
	return (List*)collection_create(type, pool, size);
}

void list_destroy(List* this) {
	void (*free)(void*) = this->free;
	free(this);
	this = NULL;
}

