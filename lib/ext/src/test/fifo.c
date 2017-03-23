#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <util/fifo.h>
#include <malloc.h>
#include <_malloc.h>

#define POOL_SIZE	0x40000
#define TEST_MAX	10000

static void fifo_create_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);
		// Checking whether fifo is in malloc_pool.
		assert_in_range(fifo, malloc_pool, malloc_pool + POOL_SIZE); 

		fifo_destroy(fifo);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_destroy_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	FIFO* fifo;
	size_t first_size = get_used_size(malloc_pool);
	fifo = fifo_create(TEST_MAX, malloc_pool);
	size_t temp_size = get_used_size(malloc_pool);

	// Checking used memory pool before and after fifo_destroy.
	assert_int_not_equal(temp_size, first_size);
	fifo_destroy(fifo);
	temp_size = get_used_size(malloc_pool);
	assert_int_equal(temp_size, first_size);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_init_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	FIFO* fifo = fifo_create(1000, malloc_pool);

	const int _SIZE = 100;
	void** array = __malloc(sizeof(void*) * _SIZE, malloc_pool);
	fifo_init(fifo, array, _SIZE);

	// Checking initialization after fifo_init.
	assert_int_equal(fifo->head, 0);
	assert_int_equal(fifo->tail, 0);
	assert_int_equal(fifo->size, _SIZE);
	assert_int_equal(fifo->array, array);
	assert_null(fifo->pool);

	fifo->pool = malloc_pool;
	fifo_destroy(fifo);
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

/* used in resize, reinit testing for void(*popped)(void*) parameter */
static void temp_popped(void* arg) {
	return;
}

static void fifo_resize_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);
		assert_int_equal(fifo->size, i);
		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
		}

		// Case 1: size up fifo array.
		fifo_resize(fifo, i + 1, temp_popped);
		assert_int_equal(fifo->size, i + 1);

		// Case 2: size down fifo array.
		fifo_resize(fifo, i - 1, temp_popped);
		assert_int_equal(fifo->size, i - 1);

		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_reinit_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
		}
		// Case 1: size up
		void* _array = fifo->array;
		void* array = __malloc((i + 1) * sizeof(void*), fifo->pool);
		fifo_reinit(fifo, array, i + 1, temp_popped);
		assert_int_equal(fifo->size, i + 1);
		__free(_array, fifo->pool);

		// Case 2: size down
		_array = fifo->array;
		array = __malloc((i - 1) * sizeof(void*), fifo->pool);
		fifo_reinit(fifo, array, i - 1, temp_popped);
		assert_int_equal(fifo->size, i - 1);
		__free(_array, fifo->pool);

		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_push_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		// Checking return value of fifo_push when fifo array is full or not.
		for(size_t j = 0; j < i - 1; j++) {
			assert_true(fifo_push(fifo, data));
		}
		assert_false(fifo_push(fifo, data));

		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_pop_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		// Checking return value of fifo_pop when fifo array is empty or not.
		assert_null(fifo_pop(fifo));
		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
		}

		for(size_t j = 0; j < i - 1; j++) {
			assert_memory_equal(fifo_pop(fifo), &data[j], sizeof(void*));
		}
		assert_null(fifo_pop(fifo));
		
		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}


static void fifo_peek_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		assert_null(fifo_peek(fifo, 0));
		
		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
		}

		for(size_t j = 0; j < i - 1; j++) {
			// Case 1: Checking whether there is data you want to check through index num in fifo array.
			assert_memory_equal(fifo_peek(fifo, j), &data[j], sizeof(void*));
		}
		// Case 2: if the second parameter is over fifo capacity.
		assert_memory_equal(fifo_peek(fifo, i), &data[0], sizeof(void*));
		
		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_size_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
			// After pushing check present data size.
			assert_int_equal(fifo_size(fifo), j + 1);
		}

		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_capacity_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);
		
		// Checking capacity.(size - 1)
		assert_int_equal(fifo_capacity(fifo), i - 1);
		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void fifo_available_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		// Checking fifo_available return value after fifo_push and fifo_pop.
		for(size_t j = 0; j < i - 1; j++) {
			assert_true(fifo_available(fifo));
			fifo_push(fifo, data);
		}

		assert_false(fifo_available(fifo));
		
		for(size_t j = 0; j < i - 1; j++) {
			fifo_pop(fifo);
			assert_true(fifo_available(fifo));
		}
		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void fifo_empty_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	void* data[TEST_MAX] = { NULL, };
	for(size_t i = 2; i < TEST_MAX; i++) {
		FIFO* fifo = fifo_create(i, malloc_pool);

		// Checking fifo_empty return value after fifo_push and fifo_pop.
		assert_true(fifo_empty(fifo));
		for(size_t j = 0; j < i - 1; j++) {
			fifo_push(fifo, data);
			assert_int_equal(fifo_size(fifo), j + 1);
		}
		assert_false(fifo_empty(fifo));
		for(size_t j = 0; j < i - 1; j++) {
			fifo_pop(fifo);
			assert_true(fifo_available(fifo));
		}
		assert_true(fifo_empty(fifo));
		

		fifo_destroy(fifo);
	}
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(fifo_create_func),
		cmocka_unit_test(fifo_destroy_func),
		cmocka_unit_test(fifo_init_func),
		cmocka_unit_test(fifo_resize_func),
		cmocka_unit_test(fifo_reinit_func),
		cmocka_unit_test(fifo_push_func),
		cmocka_unit_test(fifo_pop_func),
		cmocka_unit_test(fifo_peek_func),
		cmocka_unit_test(fifo_size_func),
		cmocka_unit_test(fifo_capacity_func),
		cmocka_unit_test(fifo_available_func),
		cmocka_unit_test(fifo_empty_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
