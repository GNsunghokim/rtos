#ifndef __UTIL_MAP_H__
#define __UTIL_MAP_H__

#include <util/base.h>
#include <util/set.h>

typedef struct _MapEntry {
	void*	key;
	void*	data;
} MapEntry;

typedef struct _Map Map;

typedef struct _MapOps {
	bool	(*is_empty)(void* this);
	void*	(*get)(void* this, void* key);
	bool	(*put)(void* this, void* key, void* value);
	bool	(*update)(void* this, void* key, void* value);
	void*	(*remove)(void* this, void* key);
	bool	(*contains_key)(void* this, void* key);
	bool	(*contains_value)(void* this, void* value);

	Set*	(*values)(void* this);
	Set*	(*entry_set)(void* this);
	Set*	(*key_set)(void* this);
} MapOps;

typedef struct _Map {
	Base;

	MapOps;

	Set*	set;
	size_t	size;
} Map;

Map* map_create(DataType type, PoolType pool, size_t size);
void map_destroy(Map* this);

#endif /* __UTIL_MAP_H__ */
