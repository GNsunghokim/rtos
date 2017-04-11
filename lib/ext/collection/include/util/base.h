#ifndef __UTIL_BASE_H__
#define __UTIL_BASE_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	DATATYPE_INT32,
	DATATYPE_INT64,
	DATATYPE_UINT32,
	DATATYPE_UINT64,
	DATATYPE_STRING,

	DATATYPE_CUSTOM_START,
	DATATYPE_CUSTOM_END = DATATYPE_CUSTOM_START + 10,

	DATATYPE_COUNT,
} DataType;

typedef enum {
	POOLTYPE_LOCAL,
	POOLTYPE_GLOBAL,

	POOLTYPE_COUNT,
} PoolType;

typedef struct _DataOps {
	uintptr_t	(*hash)(void*);
	bool		(*equals)(void*, void*);
	int		(*compare)(void*, void*);
} DataOps;

typedef struct _PoolOps {
	void*		(*malloc)(size_t);
	void		(*free)(void*);
	void*		(*calloc)(size_t, size_t);
	void*		(*realloc)(void*, size_t);
} PoolOps;

typedef struct _Base {
	DataType type;
	DataOps;

	PoolType pool;
	PoolOps;
} Base;

int register_type(DataType type, uint64_t (*hash)(void*),
		bool (*equals)(void*, void*), int (*compare)(void*, void*));
int register_pool(PoolType pool, void* (*malloc)(size_t), void* (*free)(void*),
		void* (*calloc)(size_t, size_t), void* (*realloc)(void*, size_t));

DataOps* data_ops(DataType type);
PoolOps* pool_ops(PoolType pool);

#endif /* __UTIL_BASE_H__ */
