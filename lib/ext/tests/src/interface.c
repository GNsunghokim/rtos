#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <net/interface.h>
#include <_malloc.h>
#include <malloc.h>

#define POOL_SIZE	0x40000		//random value

static void interface_alloc_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	IPv4Interface* interface = interface_alloc(malloc_pool);
	assert_in_range(interface, malloc_pool, malloc_pool + POOL_SIZE);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void interface_free_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	size_t first_size = get_used_size(malloc_pool);

	IPv4Interface* interface = interface_alloc(malloc_pool);

	size_t comp_size = get_used_size(malloc_pool);
	assert_int_not_equal(comp_size, first_size);

	interface_free(interface, malloc_pool);

	comp_size = get_used_size(malloc_pool);
	assert_int_equal(comp_size, first_size);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(interface_alloc_func),
		cmocka_unit_test(interface_free_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
