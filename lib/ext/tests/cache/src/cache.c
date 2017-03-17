#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <stdio.h>
#include <malloc.h>
#include <util/cache.h>

/* random values */
#define POOL_SIZE			0x30000		
#define CACHE_CAPACITY		0x400
#define ENTRY_SIZE			100

static int key[ENTRY_SIZE];
static int data[ENTRY_SIZE];

static void key_data_set() {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		key[i] = i;
		data[i] = i;
	}
}

static void cache_create_func(void **state) {
	Cache* cache;
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	for(size_t i = 1; i < 16385; i++) {
		cache = cache_create(i, NULL, malloc_pool);
		
		assert_in_range(cache, cache, cache + POOL_SIZE);

		cache_destroy(cache);
		
		cache = NULL;
	}

	// Case 2: If cache capacity is over map capacity. (map doesn't be created)
	cache = cache_create(16386, NULL, malloc_pool);
	assert_null(cache);


	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_destroy_func(void **state) {
	Cache* cache;
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);
	for(size_t i = 1; i < 10000; i++) {
		size_t mem_size = i;
		size_t temp_size;
		cache = cache_create(mem_size, NULL, malloc_pool);
		temp_size = get_used_size(malloc_pool);

		// Case 1: Allocated memory pool size is different from first memory pool size.
		assert_int_not_equal(temp_size, first_size);
		
		cache_destroy(cache);

		// Case 2: If cache is destroyed, present memory pool size is same with first memory pool size.
		temp_size = get_used_size(malloc_pool);
		assert_int_equal(temp_size, first_size);
		cache = NULL;
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_get_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);
	
	// Case 1: There are no data in cache.
	assert_null(cache_get(cache, (void*)&key[0]));

	// Case 2: There are data you want in cache.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
		assert_memory_equal(cache_get(cache, (void*)&key[i]), (void*)&data[i], sizeof(void*));
	}

	// Case 2: When key value is same but, address is different.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(cache_get(cache, (void*)&i));
	}
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_set_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	// Case 1: Cache capacity is over then ENTRY_SIZE.
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1-1: if there is no key in cache, return true.
		assert_true(cache_set(cache, (void*)&key[i], (void*)&data[i]));
	}
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1-2: if there is key in cache, return false.
		assert_false(cache_set(cache, (void*)&key[i], (void*)&data[i]));
	}
	cache_destroy(cache);

	// Case 2: Cache capacity is under then ENTRY_SIZE. In this test, cache capacity is half ENTRY_SIZE.
	cache = cache_create(ENTRY_SIZE / 2, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(cache_set(cache, (void*)&key[i], (void*)&data[i]));
	}

	for(int i = 0; i < ENTRY_SIZE / 2; i++) {
		// Case 2-1: There are not key&data sets[0 ~ 49] in cache.
		assert_null(cache_get(cache, (void*)&key[i]));
		// Case 2-2: There are key&data sets[50 ~ 99] in cache.
		assert_memory_equal(cache_get(cache, (void*)&key[i + 50]), (void*)&data[i + 50], sizeof(void*));
	}
	cache_destroy(cache);
	cache = NULL;
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


static void cache_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_memory_equal(cache_remove(cache, (void*)&key[i]), (void*)&data[i], sizeof(void*));
		assert_null(cache_get(cache, (void*)&key[i]));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void cache_clear_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_clear(cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_null(cache_get(cache, (void*)&key[i]));
	}
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

}

static void cache_iterator_init_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);
	
	assert_int_equal(iter.cache, cache);
	assert_int_equal(iter.node, cache->list->head);

	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;

	
}

static void cache_iterator_has_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);
	
	assert_false(cache_iterator_has_next(&iter));

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_iterator_init(&iter, cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(cache_iterator_has_next(&iter));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}
static void cache_iterator_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Cache* cache = cache_create(CACHE_CAPACITY, NULL, malloc_pool);

	CacheIterator iter;
	cache_iterator_init(&iter, cache);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		cache_set(cache, (void*)&key[i], (void*)&data[i]);
	}
	
	cache_iterator_init(&iter, cache);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_int_equal(cache_iterator_next(&iter), (void*)&data[i]);
	}
	for(int i = 707; i < 1000; i++) {
		assert_null(cache_iterator_next(&iter));
	}
	
	cache_destroy(cache);
	cache = NULL;

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(cache_create_func),
		cmocka_unit_test(cache_destroy_func),
		cmocka_unit_test(cache_get_func),
		cmocka_unit_test(cache_set_func),
		cmocka_unit_test(cache_remove_func),
		cmocka_unit_test(cache_clear_func),
		cmocka_unit_test(cache_iterator_init_func),
		cmocka_unit_test(cache_iterator_has_next_func),
		cmocka_unit_test(cache_iterator_next_func),
	};
	key_data_set();
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}

