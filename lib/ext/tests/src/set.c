#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <malloc.h>
#include <util/set.h>

#define POOL_SIZE	0x40000
#define MAX_BUCKET_SIZE 0x4000
#define ENTRY_SIZE	100
#define STRING_SIZE	8

static int data[ENTRY_SIZE];
static int same_data[ENTRY_SIZE];
static char string_data[ENTRY_SIZE][STRING_SIZE];

/* 
 * This function is made for entries to test set function.
 */
static void data_set() {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		data[i] = i;
		snprintf(string_data[i], STRING_SIZE, "Data %d", i);
	}
	for(int i = 0; i < ENTRY_SIZE / 2; i++) {
		same_data[i % 50] = i;
		same_data[i % 50 + 50] = i;
	}
}

/*
 * This function is made for calculate capacity.
 */
static inline size_t cal_capacity(size_t initial_capacity) {
	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;
	return capacity;
}

/* 
 * This function is made for comparing return value of set_string_has
 * All logic is same with set_string_has
 */
static uint64_t test_set_string_hash(void* data) {
         char* c = data;
         uint32_t len = 0;
         uint32_t sum = 0;
         while(*c != '\0') {
                 len++;
                 sum += *c++;
         }
 
         return ((uint64_t)len) << 32 | (uint64_t)sum;
}

static void set_create_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	for(int i = 0; i < MAX_BUCKET_SIZE; i++) {
		Set* set = set_create(i, NULL, NULL, malloc_pool);
		// Checking where set is in memory pool.
		assert_in_range(set, malloc_pool, malloc_pool + POOL_SIZE);
		assert_int_equal(set_capacity(set), cal_capacity(i));
		set_destroy(set);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_destroy_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);
	for(int i = 0; i < MAX_BUCKET_SIZE; i++) {
		Set* set = set_create(i, NULL, NULL, malloc_pool);

		size_t mem_size = get_used_size(malloc_pool);
		// Checking memory pool size of first and after creating set.
		assert_int_not_equal(mem_size, first_size);

		set_destroy(set);

		mem_size = get_used_size(malloc_pool);
		// Checking first size of memory pool is same with present get used size after destrying set.
		assert_int_equal(mem_size, first_size);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_is_empty_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	// Case 1: There is no date in set, just after creating set.
	assert_true(set_is_empty(set));

	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
		// Case 2: There is a data in set, return false.
		assert_false(set_is_empty(set));
		set_remove(set, (void*)&data[i]);
		// Case 3: There is no data after removing data in set.
		assert_true(set_is_empty(set));
	}
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_put_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	
	// Case 1: Set's 2nd and 3rd parameter(hash and equals) are NULL
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1-1: There is no data, so there is no same data, then return true.
		assert_true(set_put(set, (void*)&data[i]));
		// Case 1-2: There is the same data, then return false.
		assert_false(set_put(set, (void*)&data[i]));
	}
	set_destroy(set);

	// Case 2: Set's 2nd and 3rd parameter(hash and equals) are string_hash
	set = set_create(MAX_BUCKET_SIZE, set_string_hash, set_string_equals, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(set_put(set, string_data[i]));
		assert_false(set_put(set, string_data[i]));
	}
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_get_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	// Case 1: Set's 2nd and 3rd parameter(hash and equals) are NULL
	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1-1: There is no data in set, then return NULL
		assert_null(set_get(set, (void*)&data[i]));
		set_put(set, (void*)&data[i]);
		// Case 1-2: There is the data you want to get data.
		assert_memory_equal(set_get(set, (void*)&data[i]), &data[i], sizeof(void*));
	}
	set_destroy(set);

	// Case 2: Set's 2nd and 3rd parameter(hash and equals) are string_hash
	set = set_create(MAX_BUCKET_SIZE, set_string_hash, set_string_equals, malloc_pool);
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		char comp_string[STRING_SIZE];
		snprintf(comp_string, STRING_SIZE, "Data %d", i);
		assert_null(set_get(set, (void*)&string_data[i]));
		set_put(set, (void*)&string_data[i]);
		// Case 2-2: same data, but different address.
		assert_memory_equal(set_get(set, (void*)&string_data[i]), comp_string, sizeof(void*));
	}
	set_destroy(set);
	
	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_contains_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	// Case 1: Putting not null data&data set.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(set_contains(set, (void*)&data[i]));
		set_put(set, (void*)&data[i]);
		assert_true(set_contains(set, (void*)&data[i]));
	}
	set_destroy(set);

	// Case 2: Putting data with NULL data.
	set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_false(set_contains(set, (void*)&data[i]));
		set_put(set, (void*)&data[i]);
		assert_true(set_contains(set, (void*)&data[i]));
	}
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1: There is no data to remove in set.
		assert_false(set_remove(set, (void*)&data[i]));
		set_put(set, (void*)&data[i]);
		// Case 2: There is the data you want to remove in set, then return true.
		assert_memory_equal(set_remove(set, (void*)&data[i]), &data[i], sizeof(void*));
	}
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_capacity_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set;

	// Checking capacity. 
	for(size_t i = 0; i < MAX_BUCKET_SIZE; i++) {
		set = set_create(i, NULL, NULL, malloc_pool);
		assert_int_equal(set_capacity(set), cal_capacity(i));
		set_destroy(set);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_size_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	// Case 1: There is no data in set.	
	assert_int_equal(set_size(set), 0);

	// Case 2: Checking set_size through out putting and removeing data.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
		set_put(set, (void*)&data[i]);
		assert_int_equal(set_size(set), i + 1);
	}
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_remove(set, (void*)&data[i]);
		assert_int_equal(set_size(set), 99 - i);
	}
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_iterator_init_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	SetIterator iter;

	// Case 1: Iterating in condition of no data in set.
	set_iterator_init(&iter, set);
	assert_int_equal(iter.list_index, 0);	
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
	}
		
	// Case 2: Iterating in condition of being data in set.
	set_iterator_init(&iter, set);
	assert_int_equal(iter.list_index, 0);	
		
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_iterator_has_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	SetIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
	}
		
	set_iterator_init(&iter, set);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		// Case 1: There are data in set, so return true.
		assert_true(set_iterator_has_next(&iter));
		set_iterator_next(&iter);
	}

	// Case 2: The number of iterator is same with the number of putting data, so in this case return false. 
	assert_false(set_iterator_has_next(&iter));
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL; 
}

static void set_iterator_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	SetIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
	}
		
	set_iterator_init(&iter, set);

	// To usd set_iterator_next, set_iterator_has_next must be called.
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_iterator_has_next(&iter);
		size_t before_list_index = iter.list_index;
		assert_non_null(set_iterator_next(&iter));
		
		assert_int_equal(before_list_index + 1, iter.list_index);
	}
	
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_iterator_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	Set* set = set_create(MAX_BUCKET_SIZE, NULL, NULL, malloc_pool);
	SetIterator iter;
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_put(set, (void*)&data[i]);
	}
		
	set_iterator_init(&iter, set);

	for(int i = 0; i < ENTRY_SIZE; i++) {
		set_iterator_has_next(&iter);
		size_t before_list_index = iter.list_index;
		size_t before_set_size = set->size;
		assert_non_null(set_iterator_next(&iter));
		assert_non_null(set_iterator_remove(&iter));
		
		assert_int_equal(before_list_index, iter.list_index);
		assert_int_equal(before_set_size - 1, set->size);
	}
	
	set_destroy(set);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void set_uint64_hash_func(void **state) {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_int_equal((uintptr_t)&data[i], set_uint64_hash(&data[i]));
	}
}

static void set_uint64_equals_func(void **state) {
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_true(set_uint64_equals(&data[i], &data[i]));
		assert_false(set_uint64_equals(&data[i], &i));
	}
}

static void set_string_hash_func(void **state) {
	
	for(int i = 0; i < ENTRY_SIZE; i++) {
		assert_int_equal(set_string_hash(&string_data[i]), test_set_string_hash(&string_data[i]));
	}	
}

static void set_string_equals_func(void **state) {

	for(int i = 0; i < ENTRY_SIZE; i++) {
		static char temp_string_data[STRING_SIZE];
		snprintf(temp_string_data, STRING_SIZE, "Data %d", i);
		assert_true(set_string_equals(&string_data[i], temp_string_data));
		assert_false(set_string_equals(&string_data[i], "false"));
	}
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(set_create_func),
		cmocka_unit_test(set_destroy_func),
		cmocka_unit_test(set_is_empty_func),
		cmocka_unit_test(set_put_func),
		cmocka_unit_test(set_get_func),
		cmocka_unit_test(set_contains_func),
		cmocka_unit_test(set_remove_func),
		cmocka_unit_test(set_capacity_func),
		cmocka_unit_test(set_size_func),
		cmocka_unit_test(set_iterator_init_func),
		cmocka_unit_test(set_iterator_has_next_func),
		cmocka_unit_test(set_iterator_next_func),
		cmocka_unit_test(set_iterator_remove_func),
		cmocka_unit_test(set_uint64_hash_func),
		cmocka_unit_test(set_uint64_equals_func),
		cmocka_unit_test(set_string_hash_func),
		cmocka_unit_test(set_string_equals_func),
	};
	data_set();
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
