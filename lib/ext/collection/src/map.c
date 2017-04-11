#include <util/map.h>

static bool is_empty(void* this) {
	return ((Map*)this)->size == 0;
}

Map* map_create(DataType type, PoolType pool, size_t size) {
	DataOps* data_operations = data_ops(type);
	if(!data_operations)
		return NULL;

	PoolOps* pool_operations = pool_ops(pool);
	if(!pool_operations)
		return NULL;

	Map* map = pool_operations->malloc(size);
	if(!map)
		return NULL;

	map->set = set_create(DATATYPE_UINT64, pool, sizeof(Set));
	if(!map->set) {
		map->free(map);
		return NULL;
	}

	map->type	= type;
	map->pool	= pool;

	map->hash	= data_operations->hash;
	map->equals	= data_operations->equals;
	map->compare	= data_operations->compare;

	map->malloc	= pool_operations->malloc;
	map->free	= pool_operations->free;
	map->calloc	= pool_operations->calloc;
	map->realloc	= pool_operations->realloc;

	map->size	= 0;

	map->is_empty	= is_empty;

	return map;
}

void map_destroy(Map* this) {
	void (*free)(void*) = this->free;
	free(this->set);
	free(this);
	this = NULL;
}
