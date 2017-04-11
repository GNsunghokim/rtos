#include <string.h>
#include <util/set.h>

Set* set_create(DataType type, PoolType pool, size_t size) {
	return (Set*)collection_create(type, pool, size);
}

void set_destroy(Set* this) {
	void (*free)(void*) = this->free;
	free(this);
	this = NULL;
}

