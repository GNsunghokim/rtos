#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <fio.h>
#include <_malloc.h>
#include <tlsf.h>

#include <malloc.h>

extern void* __malloc_pool;

static void fio_create_func() {
	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	FIO* fio = fio_create(__malloc_pool);
	assert_in_range(fio, __malloc_pool, __malloc_pool + 0x40000);	
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(fio_create_func)
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
