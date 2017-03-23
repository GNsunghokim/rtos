#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <gmalloc.h>

#include <tlsf.h>
#include <stdio.h>
#include <malloc.h>

extern void* __gmalloc_pool;

static void gmalloc_func() {
	size_t pool_size = 0x40000;

	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		__gmalloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __gmalloc_pool, 0);

		void* temp = gmalloc(i);
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		destroy_memory_pool(__gmalloc_pool);		
		free(__gmalloc_pool);
		__gmalloc_pool = NULL;
	}
}

static void gfree_func() {
	size_t pool_size = 0x40000;
	__gmalloc_pool = malloc(pool_size);
	init_memory_pool(pool_size, __gmalloc_pool, 0);	

	void* temp = gmalloc(0x3e000);
	assert_non_null(temp);
	void* temp2 = gmalloc(0x3e000);		
	assert_null(temp2);

	gfree(temp);
	temp = gmalloc(0x3e000);

	assert_non_null(temp);

	destroy_memory_pool(__gmalloc_pool);
	free(__gmalloc_pool);
	__gmalloc_pool = NULL;
}

static void grealloc_func() {
	size_t pool_size = 0x40000;
	
	for(int i = 1; i < 0x40000 - 0x3e001; i++) {
		__gmalloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __gmalloc_pool, 0);	
		
		void* temp = gmalloc(i);	
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		temp = grealloc(temp, 0x3e000);			
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		gfree(temp);
		temp = NULL;

		destroy_memory_pool(__gmalloc_pool);
		free(__gmalloc_pool);
		__gmalloc_pool = NULL;
	}	
}

static void gcalloc_func() {
	size_t pool_size = 0x40000;

	/* element size 1*/
	for(size_t i = 1; i < pool_size - 0x3e001; i++) {
		__gmalloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __gmalloc_pool, 0);

		char* temp = gcalloc(i, 1);
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		for(size_t j = 0; j < i; j++)
			assert_int_equal(0, temp[j]);
		
		destroy_memory_pool(__gmalloc_pool);
		free(__gmalloc_pool);
		__gmalloc_pool = NULL;
	}

	/* element size 4*/
	for(size_t i = 1; i < (pool_size - 0x3e001) / 4; i++) {
		__gmalloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __gmalloc_pool, 0);

		int* temp = gcalloc(i, 4);
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		for(size_t j = 0; j < i; j++)
			assert_int_equal(0, temp[j]);
		
		destroy_memory_pool(__gmalloc_pool);
		free(__gmalloc_pool);
		__gmalloc_pool = NULL;
	}

	/* element size 8*/
	for(size_t i = 1; i < (pool_size - 0x3e001) / 8; i++) {
		__gmalloc_pool = malloc(pool_size);
		init_memory_pool(pool_size, __gmalloc_pool, 0);

		int* temp = gcalloc(i, 8);
		assert_in_range(temp, __gmalloc_pool, __gmalloc_pool + pool_size);

		for(size_t j = 0; j < i; j++)
			assert_int_equal(0, temp[j]);
		
		destroy_memory_pool(__gmalloc_pool);
		free(__gmalloc_pool);
		__gmalloc_pool = NULL;
	}
}

int main() {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(gmalloc_func),
		cmocka_unit_test(gfree_func),
		cmocka_unit_test(grealloc_func),
		cmocka_unit_test(gcalloc_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
