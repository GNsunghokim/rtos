#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <_malloc.h>

#include <malloc.h>
#include <util/list.h>

//40000, 5329

#define POOL_SIZE	0x40000
#define ENTRY_SIZE	5329    // Depending of POOL_SIZE. 0x40000 -> Max number of list(no data) : 5329
#define DATA_SIZE	1000

static void list_create_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list[ENTRY_SIZE];

	for(int j = 0; j < ENTRY_SIZE; j++) {
		list[j] = list_create(malloc_pool);
	}
	// Case 1: If list_create function fails, return NULL.
	// POOL_SIZE = 0x40000, the number of list without data is upto 5329;
	List* list_NULL = list_create(malloc_pool);
	assert_null(list_NULL);

	for(int j = 0; j < ENTRY_SIZE; j++) {
		// Case 2: Checking list[] which are created whether in range of malloc_pool.
		assert_in_range(list[j], malloc_pool, malloc_pool + POOL_SIZE);     
		list_destroy(list[j]);
	}
	destroy_memory_pool(malloc_pool);

	free(malloc_pool);
}

static void list_destroy_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list;

	size_t first_size = get_used_size(malloc_pool);
	for(int i = 0; i < 5000; i++) {
		list = list_create(malloc_pool);
		size_t mem_size = get_used_size(malloc_pool);

		// Case 1: comparing first and after creating list used memory size in malloc_pool.
		assert_int_not_equal(mem_size, first_size);     
		list_destroy(list);

		mem_size = get_used_size(malloc_pool);

		// Case 2: comparing first and after destroying list used memory size in malloc_pool.
		assert_int_equal(mem_size, first_size);
	}

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}	

static void list_is_empty_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);
	List* list;

	list = list_create(malloc_pool);
	// Case 1: After creating list, there is no data in list.
	assert_true(list_is_empty(list));

	int data[DATA_SIZE];
	for(int j = 0; j < DATA_SIZE; j++) {
		list_add(list, (void*)&data[j]);
	}
	// Case 2: After adding data in list, return false.
	assert_false(list_is_empty(list));

	for(int j = 0; j < DATA_SIZE; j++) {
		list_remove_first(list);
	}
	// Case 3: After removing all data in list, return true.
	assert_true(list_is_empty(list));
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_add_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);
	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
		// adding data is located at the last of list.
		assert_memory_equal(list_get_last(list), (void*)&data[i], sizeof(void*));
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

/* NOTE: list_add_at_func
 *  - For putting in data at random index, this test code uses dummy_index[].
 *  - Every 10 times in for-loop, each dummy_index values increases 3. 
 *  - increasing size(3) is randomly selected size. (but, this size must be under 10)
 */
static void list_add_at_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int dummy_index[10] = { 0, 5, 2, 7, 3, 4, 9, 6, 1, 8 };
	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		int index = dummy_index[i % 10];

		list_add_at(list, index, (void*)&data[i]);

		// In this list, the size of list is same with 'i', because every for-loop list is added 1 entry.
		if(i >= index) // Case 1: If the full size of list(i) is over then index, it means the data you want to add in list is located along with your object index number.
			assert_memory_equal(list_get(list, index), (void*)&data[i], sizeof(void*));
		else  // Case 2: If i is under then index, it always adds at last node.
			assert_memory_equal(list_get_last(list), (void*)&data[i], sizeof(void*));

		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] += 3;
			}
		}
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

/* NOTE: list_get_func
 *  - Using same logic with list_add_at_func, this test code approach data through random index.
 */
static void list_get_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int dummy_index[10] = { 0, 5, 2, 7, 3, 4, 9, 6, 1, 8 };
	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		int index = dummy_index[i % 10];

		list_add_at(list, index, (void*)&data[i]);
		if(i >= index)
			assert_memory_equal(list_get(list, index), (void*)&data[i], sizeof(void*));
		else	// If your object index is over then list index number, return NULL.
			assert_null(list_get(list, index));

		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] += 3;
			}
		}
	}
	for(int i = DATA_SIZE; i >= 0; i--) {
		int index = dummy_index[i % 10];
		void* comp_data = list_get(list, index);

		if(i >= index)
			assert_memory_equal(list_remove(list, index), comp_data, sizeof(void*));
		else
			assert_null(list_remove(list, index));

		if(i % 10 == 0) {
			for(int j = 0; j < 10; j++) {
				dummy_index[j] -= 3;
			}
		}
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_get_first_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	assert_null(list_get_first(list));

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add_at(list, 0, (void*)&data[DATA_SIZE - 1 - i]);
		assert_memory_equal(list_get_first(list), (void*)&data[DATA_SIZE - 1 - i], sizeof(void*));
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		list_remove_first(list);

		// There is no entry in list.
		if(i == DATA_SIZE - 1) {
			assert_null(list_get_first(list));
			break;
		}

		assert_memory_equal(list_get_first(list), (void*)&data[i + 1], sizeof(void*));
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_get_last_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);
	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
		assert_memory_equal(list_get_last(list), (void*)&data[i], sizeof(void*));
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		list_remove_last(list);

		// There is no entry in list.
		if(i == DATA_SIZE - 1) {
			assert_null(list_get_last(list));
			break;
		}

		assert_memory_equal(list_get_last(list), (void*)&data[DATA_SIZE - 2 - i], sizeof(void*));
	}
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
}

static void list_index_of_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		// Case 1: If there is no data you want in list, return -1.
		assert_int_equal(list_index_of(list, (void*)&data[i], NULL), -1);

		list_add(list, (void*)&data[i]);
		// Case 2: If there is the data you want in list, return index.
		assert_int_equal(list_index_of(list, (void*)&data[i], NULL), i);
	}

	assert_int_equal(list_index_of(list, NULL, NULL), -1);

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_memory_equal(list_remove(list, DATA_SIZE - 1 - i), &data[DATA_SIZE - 1 - i], sizeof(void*));
		assert_int_equal(list_index_of(list, &data[DATA_SIZE - 1 - i], NULL), -1);
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_data_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		data[i] = i;
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_true(list_remove_data(list, &data[i]));
		assert_false(list_remove_data(list, &data[i]));
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_first_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	assert_null(list_remove_first(list));

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_memory_equal(list_remove_first(list), &data[i], sizeof(void*));
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_remove_last_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	assert_null(list_remove_last(list));

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_memory_equal(list_remove_last(list), &data[DATA_SIZE - 1 - i], sizeof(void*));
	}

	assert_null(list_remove_last(list));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_size_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		assert_int_equal(list_size(list), i);
		list_add(list, (void*)&data[i]);
		assert_int_equal(list_size(list), i + 1);
	}

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_rotate_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_memory_equal(list_get_first(list), &data[i], sizeof(void*));
		assert_memory_equal(list_get_last(list), &data[(i + DATA_SIZE - 1) % DATA_SIZE], sizeof(void*));
		list_rotate(list);
		assert_memory_equal(list_get_first(list), &data[(i + 1) % DATA_SIZE], sizeof(void*));

		assert_memory_equal(list_get_last(list), &data[i], sizeof(void*));
	}		
	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_init_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_memory_equal(iter.list, list, sizeof(List*));
	assert_null(iter.prev);
	assert_memory_equal(iter.node, list->head, sizeof(ListNode*));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_has_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_false(list_iterator_has_next(&iter));

	void* data = malloc(sizeof(void*));
	list_add(list, data);

	list_iterator_init(&iter, list);

	assert_true(list_iterator_has_next(&iter));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	

}

static void list_iterator_next_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);

	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);

	assert_null(list_iterator_next(&iter));

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	list_iterator_init(&iter, list);

	for(int i = 0; i < DATA_SIZE; i++) {
		assert_memory_equal(list_iterator_next(&iter), &data[i], sizeof(void*));
	}

	assert_null(list_iterator_next(&iter));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

static void list_iterator_remove_func(void **state) {
	void* malloc_pool = malloc(POOL_SIZE);

	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	List* list = list_create(malloc_pool);

	ListIterator iter;
	list_iterator_init(&iter, list);
	assert_null(list_iterator_remove(&iter));

	int data[DATA_SIZE];
	for(int i = 0; i < DATA_SIZE; i++) {
		list_add(list, (void*)&data[i]);
	}

	list_iterator_init(&iter, list);

	for(int i = 0; i < DATA_SIZE; i++) {
		void* data = list_iterator_next(&iter);
		assert_memory_equal(list_iterator_remove(&iter), data, sizeof(void*));
	}

	assert_false(list_iterator_has_next(&iter));

	list_destroy(list);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);	
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(list_create_func),
		cmocka_unit_test(list_destroy_func),
		cmocka_unit_test(list_is_empty_func),
		cmocka_unit_test(list_add_func),
		cmocka_unit_test(list_add_at_func),
		cmocka_unit_test(list_get_func),
		cmocka_unit_test(list_get_first_func),
		cmocka_unit_test(list_get_last_func),
		cmocka_unit_test(list_index_of_func),
		cmocka_unit_test(list_remove_func),
		cmocka_unit_test(list_remove_data_func),
		cmocka_unit_test(list_remove_first_func),
		cmocka_unit_test(list_remove_last_func),
		cmocka_unit_test(list_size_func),
		cmocka_unit_test(list_rotate_func),
		cmocka_unit_test(list_iterator_init_func),
		cmocka_unit_test(list_iterator_has_next_func),
		cmocka_unit_test(list_iterator_next_func),
		cmocka_unit_test(list_iterator_remove_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
