#include <stdlib.h>
#include <string.h>
#include <util/base.h>

/* Data type functions */
static uintptr_t uint64_hash(void* key) {
	return (uintptr_t)key;
}

static bool uint64_equals(void* key1, void* key2) {
	return key1 == key2;
}

static int uint64_compare(void* key1, void* key2) {
	return key1 == key2;
}

static uintptr_t string_hash(void* key) {
	char* c = key;
	uint32_t len = 0;
	uint32_t sum = 0;
	while(*c != '\0') {
		len++;
		sum += *c++;
	}

	return ((uintptr_t)len) << 32 | (uintptr_t)sum;
}

static bool string_equals(void* key1, void* key2) {
	char* c1 = key1;
	char* c2 = key2;

	while(*c1 != '\0' && *c2 != '\0') {
		if(*c1++ != *c2++)
			return false;
	}

	if(*c1 != '\0' || *c2 != '\0')
		return false;

	return true;
}

static int string_compare(void* key1, void* key2) {
	return strcmp(key1, key2);
}

static DataOps data_operations[DATATYPE_COUNT] = {
	[DATATYPE_UINT64] = {
		.hash		= uint64_hash,
		.equals		= uint64_equals,
		.compare	= uint64_compare,
	},
	[DATATYPE_STRING] = {
		.hash		= string_hash,
		.equals		= string_equals,
		.compare	= string_compare,
	},
	// TODO: implement other types
};

static PoolOps pool_operations[POOLTYPE_COUNT] = {
	// Local pool is registered by default. It is from standard library
	[POOLTYPE_LOCAL] = {
		.malloc		= malloc,
		.free		= free,
		.calloc		= calloc,
		.realloc	= realloc,
	},
};

int register_type(DataType type, uint64_t (*hash)(void*),
		bool (*equals)(void*, void*), int (*compare)(void*, void*)) {
	// Not yet implemented
	return 0;
}

int register_pool(PoolType pool, void* (*malloc)(size_t), void* (*free)(void*),
		void* (*calloc)(size_t, size_t), void* (*realloc)(void*, size_t)) {
	// Not yet implemented
	return 0;
}

DataOps* data_ops(DataType type) {
	if(type < 0 || type >= DATATYPE_COUNT)
		return NULL;

	if(!data_operations[type].hash)
		return NULL;

	return &data_operations[type];
}

PoolOps* pool_ops(PoolType pool) {
	if(pool < 0 || pool >= POOLTYPE_COUNT)
		return NULL;

	if(!pool_operations[pool].malloc)
		return NULL;

	return &pool_operations[pool];
}

/*
 *int base_init(DataType type, DataOps* data_ops, PoolType pool, PoolOps* pool_ops) {
 *        DataOps* data_operations = data_ops(type);
 *        if(!data_operations)
 *                return -INVALID_DATATYPE;
 *
 *        PoolOps* pool_operations = pool_ops(pool);
 *        if(!pool_operations)
 *                return -INVALID_POOLTYPE;
 *
 *        data_ops->hash		= data_operations->hash;
 *        data_ops->equals	= data_operations->equals;
 *        data_ops->compare	= data_operations->compare;
 *
 *        pool_ops->malloc	= pool_operations->malloc;
 *        pool_ops->free		= pool_operations->free;
 *        pool_ops->calloc	= pool_operations->calloc;
 *        pool_ops->realloc	= pool_operations->realloc;
 *
 *        return 0;
 *}
 */
