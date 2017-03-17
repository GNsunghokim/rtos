#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <thread.h>

extern int __thread_id;
extern int __thread_count;

static void thread_id_func() {
	for(int i = 0; i < 4000; i++) {
		__thread_id = i;
		assert_int_equal(thread_id(), i);
	}
}

static void thread_count_func() {
	for(int i = 0; i < 4000; i++) {
		__thread_count = i;
		assert_int_equal(thread_count(), i);
	}
}

static void thread_barrior_func() {

}

int main() {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(thread_id_func),
		cmocka_unit_test(thread_count_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
