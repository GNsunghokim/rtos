#include <util/collection.h>

static bool is_empty(void* this) {
	return ((Collection*)this)->size == 0;
}

Collection* collection_create(DataType type, PoolType pool, size_t size) {
	DataOps* data_operations = data_ops(type);
	if(!data_operations)
		return NULL;

	PoolOps* pool_operations = pool_ops(pool);
	if(!pool_operations)
		return NULL;

	Collection* collection = pool_operations->malloc(size);
	if(!collection)
		return NULL;

	collection->type	= type;
	collection->pool	= pool;

	collection->hash	= data_operations->hash;
	collection->equals	= data_operations->equals;
	collection->compare	= data_operations->compare;

	collection->malloc	= pool_operations->malloc;
	collection->free	= pool_operations->free;
	collection->calloc	= pool_operations->calloc;
	collection->realloc	= pool_operations->realloc;

	collection->size	= 0;
	collection->is_empty	= is_empty;

	return collection;
}
