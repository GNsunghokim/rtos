#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <util/vector.h>
#include <malloc.h>
#include <_malloc.h>

/* random value */
#define POOL_SIZE		0x40000
#define VECTOR_SIZE		0x400
#define ARRAY_SIZE		100

static int data[ARRAY_SIZE];

static void array_set() {
	for(int i = 0; i < ARRAY_SIZE; i++) {
		data[i] = i;
	}
}

static void vector_create_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	for(int i = 0; i < VECTOR_SIZE; i++) {
		Vector* vector = vector_create(i, malloc_pool);
		// Checking whether vector is in memory pool.
		assert_in_range(vector, malloc_pool, malloc_pool + POOL_SIZE);
		vector_destroy(vector);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_destory_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);
	for(int i = 0; i < VECTOR_SIZE; i++) {
		Vector* vector = vector_create(i, malloc_pool);

		size_t mem_size = get_used_size(malloc_pool);
		// Checking memory pool size of first and after creating vector.
		assert_int_not_equal(mem_size, first_size);

		vector_destroy(vector);

		mem_size = get_used_size(malloc_pool);
		// Checking first size of memory pool is same with present get used size after destrying vector.
		assert_int_equal(mem_size, first_size);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_init_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	size_t size = 10;
	
	// Case 1: Array made through __malloc.
	void** array1 = __malloc(size * sizeof(void*), malloc_pool);
	vector_init(vector, array1, size);
	vector->pool = malloc_pool;
	assert_int_equal(size, vector->size);
	assert_memory_equal(array1, vector->array, size * sizeof(void*));
	
	// Case 2: Array made through malloc.
	void** array2 = (void**)malloc(size * sizeof(void*));
	vector_init(vector, array2, size);
	assert_int_equal(size, vector->size);
	assert_memory_equal(array2, vector->array, size * sizeof(void*));
	
	
	destroy_memory_pool(malloc_pool);
	free(array2);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_available_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(ARRAY_SIZE, malloc_pool);

	// Case 1: vector is not full.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_true(vector_available(vector));
		vector_add(vector, (void*)&data[i]);
	}

	// Case 2: vector is full.
	assert_false(vector_available(vector));

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_is_empty_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	// Case 1: There is no data in vector.
	assert_true(vector_is_empty(vector));

	// Case 2: There is a data in vector.
	vector_add(vector, (void*)&data[0]);
	assert_false(vector_is_empty(vector));

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_add_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	Vector* vector = vector_create(ARRAY_SIZE, malloc_pool);

	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_true(vector_add(vector, (void*)&data[i]));
	}
	// Adding data into full vector.
	assert_false(vector_add(vector, (void*)&data[0]));

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_get_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(ARRAY_SIZE * 2, malloc_pool);
	
	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}

	// Checking vector index seperating into 2 part.(0~99, 100~199).
	// There are data at index 0 to 99, so checking memory address.
	// There are no data at index 100 to 199, so return null.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_memory_equal(vector_get(vector, i), (void*)&data[i], sizeof(void*));
		assert_null(vector_get(vector, i + 100));
	}

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_index_of_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	
	// Case 1: There are no data in vector.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_int_equal(vector_index_of(vector, (void*)&data[i], NULL), -1);
	}

	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}

	// Case 2: There are data you want in vector.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_int_equal(vector_index_of(vector, (void*)&data[i], NULL), i);
	}
	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_remove_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	
	// Case 1: There are no data in vector.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_null(vector_remove(vector, i));
	}

	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}

	// Case 2: There are data you want in vector.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_memory_equal(vector_remove(vector, 0), (void*)&data[i], sizeof(void*));
	}

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_size_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
		
	// Checking before&after size of vector.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_int_equal(vector_size(vector), i);
		vector_add(vector, (void*)&data[i]);
		assert_int_equal(vector_size(vector), i + 1);
	}

	vector_destroy(vector);
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_capacity_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 0; i < VECTOR_SIZE; i++) {
		Vector* vector = vector_create(i, malloc_pool);
	
		assert_int_equal(vector_capacity(vector), i);
		vector_destroy(vector);
	}
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_iterator_init_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	VectorIterator iter;

	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);

	// Checking init value of iterator(index value and vector address).
	vector_iterator_init(&iter, vector);
	assert_int_equal(iter.index, 0);
	assert_memory_equal(iter.vector, vector, sizeof(Vector*));

	vector_destroy(vector);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_iterator_has_next_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	VectorIterator iter;

	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}
	
	vector_iterator_init(&iter, vector);
	
	// After adding data and, rounding iterator, check whether next iterator is or not.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_true(vector_iterator_has_next(&iter));
		vector_iterator_next(&iter);
	}
	assert_false(vector_iterator_has_next(&iter));
	
	vector_destroy(vector);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_iterator_next_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	VectorIterator iter;

	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}
	
	vector_iterator_init(&iter, vector);
	
	while(vector_iterator_has_next(&iter)) {	
		static int i = 0;
		assert_memory_equal(vector_iterator_next(&iter), &data[i++], sizeof(void*));
	}
	vector_destroy(vector);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void vector_iterator_remove_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	VectorIterator iter;

	Vector* vector = vector_create(VECTOR_SIZE, malloc_pool);
	for(int i = 0; i < ARRAY_SIZE; i++) {
		vector_add(vector, (void*)&data[i]);
	}
	
	vector_iterator_init(&iter, vector);
	
	while(vector_iterator_has_next(&iter)) {	
		vector_iterator_next(&iter);
	}

	// After rounding iterator, remove last iterator, and compare return data with input data address.
	for(int i = 0; i < ARRAY_SIZE; i++) {
		assert_memory_equal(vector_iterator_remove(&iter), &data[ARRAY_SIZE - 1 - i], sizeof(void*));
	}
	vector_destroy(vector);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(vector_create_func),
		cmocka_unit_test(vector_destory_func),
		cmocka_unit_test(vector_init_func),
		cmocka_unit_test(vector_available_func),
		cmocka_unit_test(vector_is_empty_func),
		cmocka_unit_test(vector_add_func),
		cmocka_unit_test(vector_get_func),
		cmocka_unit_test(vector_index_of_func),
		cmocka_unit_test(vector_remove_func),
		cmocka_unit_test(vector_size_func),
		cmocka_unit_test(vector_capacity_func),
		cmocka_unit_test(vector_iterator_init_func),
		cmocka_unit_test(vector_iterator_has_next_func),
		cmocka_unit_test(vector_iterator_next_func),
		cmocka_unit_test(vector_iterator_remove_func),
	};
	array_set();
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
