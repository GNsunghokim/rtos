#include <_malloc.h>
#include <util/cache.h>

Cache* cache_create(size_t capacity, void(*uncache)(void*), void* pool) {
	if(capacity == 0)
		return NULL;

	Cache* cache = __malloc(sizeof(Cache), pool);
	if(!cache)
		return NULL;
	
	cache->map = map_create(capacity, NULL, NULL, pool);
	if(!cache->map) {
		__free(cache, pool);
		return NULL;
	}
	
	cache->list = list_create(pool);
	if(!cache->list) {
		map_destroy(cache->map);
		__free(cache, pool);
		return NULL;
	}

	cache->capacity = capacity;
	cache->uncache = uncache;
	cache->pool = pool;

	return cache;
}

void cache_destroy(Cache* cache) {
	cache_clear(cache);

	list_destroy(cache->list);
	map_destroy(cache->map);

	__free(cache, cache->pool);
}

void* cache_get(Cache* cache, void* key) {
	void* data = map_get(cache->map, key);
	if(!data) 
		return NULL;
	
	// Update cache ordering
	list_remove_data(cache->list, key);
	list_add(cache->list, key);
	
	return data;
}

bool cache_set(Cache* cache, void* key, void* data) {
	// Check existed data having the key
	if(map_contains(cache->map, key))
		return false;
	
	if(list_size(cache->list) >= cache->capacity) {
		// Delete LRU data  
		void* lru_key = list_remove_first(cache->list);
		void* lru_data = map_remove(cache->map, lru_key);
		if(cache->uncache)
			cache->uncache(lru_data);
	}
	
	list_add(cache->list, key);
	map_put(cache->map, key, data);

	return true;
}

void* cache_remove(Cache* cache, void* key) {
	void* data = map_remove(cache->map, key);
	if(!data) 
		return NULL;

	list_remove_data(cache->list, key);
	if(cache->uncache)
		cache->uncache(data);
	
	return data;
}

void cache_clear(Cache* cache) {
	while(list_size(cache->list) > 0) {
		void* key = list_remove_first(cache->list);
		void* data = map_remove(cache->map, key);
		if(cache->uncache)
			cache->uncache(data);
	}
}

void cache_iterator_init(CacheIterator* iter, Cache* cache) {
	iter->cache = cache;
	iter->node = cache->list->head;
}

bool cache_iterator_has_next(CacheIterator* iter) {
	return iter->node != NULL;
}

void* cache_iterator_next(CacheIterator* iter) {
	if(iter->node) {
		void* key = iter->node->data;
		iter->node = iter->node->next;
	
		return map_get(iter->cache->map, key);
	} else {
		return NULL;
	}
}
