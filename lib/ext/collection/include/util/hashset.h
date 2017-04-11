#ifndef __UTIL_HASH_SET_H__
#define __UTIL_HASH_SET_H__

#include <util/set.h>
#include <util/hashmap.h>

typedef struct _HashSet {
	Set;

        HashMap*    map;
} HashSet;

#endif /* __UTIL_HASH_SET_H__ */
