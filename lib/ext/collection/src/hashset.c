#include <util/hashset.h>

HashSet* hashset_create(DataType type, PoolType pool, size_t initial_capacity) {
	HashSet* set = (HashSet*)set_create(type, pool, sizeof(HashSet));
	if(!set)
		return NULL;

	set->capacity	= initial_capacity;
	set->map	= hashmap_create(type, pool, initial_capacity);

	return set;
}

void hashset_destroy(HashSet* this) {
	void (*free)(void*) = this->free;
	free(this->array);

	set_destroy((Queue*)this);
}

