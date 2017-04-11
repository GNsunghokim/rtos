#ifndef __UTIL_SET_H__
#define __UTIL_SET_H__

#include <util/collection.h>
#include <util/linkedlist.h>

typedef struct _SetEntry {
	void* data;
} SetEntry;

typedef struct _Set {
	Collection;

	size_t		capacity;
} Set;

Set* set_create(DataType type, PoolType pool, size_t size);
void set_destroy(Set* this);

#endif /* __UTIL_SET_H__ */
