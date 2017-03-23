#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>

/* The MAX_SIZE is defined by real execution of _malloc.
 * When the pool size is 0x40000, the maximum size that won't 
 * return NULL from _malloc is 0x3e000.
 * If the arguments of _malloc size is over the MAX_SIZE, will return NULL.
 * */
#define MAX_SIZE	0x3e000
#define POOL_SIZE	0x40000


extern void* __malloc_pool;

/*
static void malloc_const_size_null(void** state) {
	size_t POOL_SIZE = 0x40000;
	__malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, __malloc_pool, 0);
	
	size_t mem_size = 0;
	void* mem = NULL;

	mem = __malloc(mem_size, NULL);
	assert_null(mem);
	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void malloc_const_over_pool() {
	size_t POOL_SIZE = 0x40000;
	__malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, __malloc_pool, 0);

	for(int i = 1; i < 1000; i++) {
		size_t mem_size  = 0x40000 + i;
		void* mem = __malloc(mem_size, NULL);

		assert_null(mem);
	}
	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void malloc_const_over_chunk() {
	size_t POOL_SIZE = 0x40000;
	for(int i = 1; i < POOL_SIZE; i++) {
		__malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, __malloc_pool, 0);

		void* used = __malloc(i, NULL);
		void* mem = __malloc(POOL_SIZE - i + 1, NULL);

		assert_null(mem);

		free(__malloc_pool);
		__malloc_pool = NULL;
	}
}

static void malloc_stress_below_pool() {
	size_t POOL_SIZE = 0x40000;
	size_t malloc_size = 0x11;

	for(int i = 0; i < 4000; i++){
		__malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, __malloc_pool, 0);

		uint64_t sum = 0;

		while(1) {
			sum += malloc_size;
			if(sum > POOL_SIZE) {
				sum -= malloc_size;
				break;
			} else {
				void* use = __malloc(malloc_size, NULL);	
				assert_in_range(use, __malloc_pool, __malloc_pool + POOL_SIZE);
			}
		}
		
		void* last = __malloc(POOL_SIZE - sum, NULL);
		assert_in_range(last, __malloc_pool, __malloc_pool + POOL_SIZE);

		free(__malloc_pool);
		__malloc_pool = NULL;	
	}
}

static void malloc_stress_all_pool() {

}
*/

static void malloc_func(void** state) {
	/* Checking the allocated memory by _malloc is in pool memory area. */

	/* Use local pool */
	for(size_t i = 1; i < MAX_SIZE; i++) {
		void* malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, malloc_pool, 0);
		
		size_t mem_size = i;
		void* mem = __malloc(mem_size, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + POOL_SIZE);	

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}	

	/* Use __malloc_pool */
	for(size_t i = 1; i < MAX_SIZE; i++) {
		__malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, __malloc_pool, 0);
		
		size_t mem_size = i;
		void* mem = __malloc(mem_size, NULL);

		assert_in_range(mem, __malloc_pool, __malloc_pool + POOL_SIZE);	

		destroy_memory_pool(__malloc_pool);
		free(__malloc_pool);
		__malloc_pool = NULL;
	}	
}

static void free_func() {
	/* Checking the first used size of initalized pool.
	 * And compare the freed size after malloc and first used size.
	 * */

	__malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, __malloc_pool, 0);

	size_t first_size = get_used_size(__malloc_pool);
	for(int i = 1; i < MAX_SIZE; i++) {
		void* mem = __malloc(i, NULL);		
		size_t temp_size = get_used_size(__malloc_pool);
		assert_int_not_equal(temp_size, first_size);

		__free(mem, NULL);

		temp_size = get_used_size(__malloc_pool);
		assert_int_equal(temp_size, first_size);	
	}	

	destroy_memory_pool(__malloc_pool);
	free(__malloc_pool);
	__malloc_pool = NULL;
}

static void realloc_func() {
	/* Chekcing the realloc that has the size in range 
	 * from 1 to MAX SIZE is after malloc in pool area.
	 * (including smaller, same, bigger than original malloc size)
	 * */

	size_t malloc_size;

	for(size_t i = 1; i < MAX_SIZE; i++) {
		malloc_size = i;

		__malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, __malloc_pool, 0);

		void* mem = __malloc(malloc_size, NULL);		
		assert_in_range(mem, __malloc_pool, __malloc_pool + POOL_SIZE);

		for(int j = 1; j < MAX_SIZE; j++) {
			mem = __realloc(mem, j, NULL);
			assert_in_range(mem, __malloc_pool, __malloc_pool + POOL_SIZE);
		}

		destroy_memory_pool(__malloc_pool);
		free(__malloc_pool);
		__malloc_pool = NULL;
	}
}

static void calloc_func() {
	size_t malloc_size;
	/* Chekcing the whole memory area retrun from calloc is set to zero */

	/* Element size 1 */	
	for(size_t i = 1; i < MAX_SIZE; i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, malloc_pool, 0);

		char* mem = __calloc(malloc_size, 1, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + POOL_SIZE);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, (int)mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}

	/* Element size 4 */
	for(size_t i = 1; i < (MAX_SIZE / 4); i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, malloc_pool, 0);

		int* mem = __calloc(malloc_size, 4, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + POOL_SIZE);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, (int)mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}

	/* Element size 8 */
	for(size_t i = 1; i < (MAX_SIZE / 8); i++) {
		malloc_size = i;
		
		void* malloc_pool = malloc(POOL_SIZE);
		init_memory_pool(POOL_SIZE, malloc_pool, 0);

		uint64_t* mem = __calloc(malloc_size, 8, malloc_pool);
		assert_in_range(mem, malloc_pool, malloc_pool + POOL_SIZE);
	
		for(size_t j = 0; j < malloc_size; j++) 
			assert_int_equal(0, mem[j]);

		destroy_memory_pool(malloc_pool);
		free(malloc_pool);
		malloc_pool = NULL;
	}
}

int main() {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(malloc_func),
//		cmocka_unit_test(free_func),
//		cmocka_unit_test(realloc_func),
//		cmocka_unit_test(calloc_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
