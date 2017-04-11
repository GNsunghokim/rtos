#ifndef __UTIL_COLLECTION_H__
#define __UTIL_COLLECTION_H__

#include <util/base.h>

typedef struct _Iterator Iterator;
typedef struct _Iterator {
	void		(*init)(void* context, void* collection);
	bool		(*has_next)(void* context);
	void*		(*next)(void* context);
	void*		(*remove)(void* context);
} Iterator;

typedef struct _Collection Collection;

typedef struct _CollectionOps {
	bool		(*is_empty)(void* this);
	bool		(*contains)(void* this, void* element);
	bool		(*add)(void* this, void* element);
	bool		(*remove)(void* this, void* element);
} CollectionOps;

typedef struct _Collection {
	Base;

	CollectionOps;

	size_t		size;
	Iterator*	iter;
} Collection;

Collection* collection_create(DataType type, PoolType pool, size_t size);

// FIXME: make '__context' and '__iter' local variable in for loop scope
#define for_each(element, collection)			\
	typeof(*(collection)->context) __context;	\
	Iterator* __iter = (collection)->iter;		\
	__iter->init(&__context, (collection));		\
	for((element) = __iter->next(&__context);	\
		__iter->has_next(&__context) == true;	\
		(element) = __iter->next(&__context))

#endif /* __UTIL_COLLECTION_H__ */
