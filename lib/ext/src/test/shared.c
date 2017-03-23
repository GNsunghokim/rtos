#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <shared.h>

#include <malloc.h>
extern void** __shared;

static void shared_set_func() {
	__shared = malloc(sizeof(void*));

	for(int i = 1; i < 4000; i++) {
		void* temp = malloc(i);					
		shared_set(temp);
		
		assert_int_equal(temp, *__shared);

		free(temp);
		temp = NULL;
	}

	free(__shared);
	__shared = NULL;
}

static void shared_get_func() {
	__shared = malloc(sizeof(void*));

	for(int i = 1; i < 4000; i++) {
		void* temp = malloc(i);
		shared_set(temp);
	
		assert_int_equal(shared_get(), temp);	

		free(temp);
		temp = NULL;	
	}
	
	__shared = NULL;
}

int main() {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(shared_set_func),
		cmocka_unit_test(shared_get_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
