#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdint.h>
#include <lock.h>

#include <pthread.h>
#define THREAD_NUMBER	100

static uint8_t volatile lock;
static int shared_data = 0;
static pthread_t thds[THREAD_NUMBER];

void* thread_lock(void* arg);

static void lock_init_func(void** state) {
	lock = 1;
	assert_int_not_equal(lock, 0);

	lock_init(&lock);
	assert_int_equal(lock, 0);
}

/* For lock function test, it needs to use both lock_lock and lock_unlock function, so both functions are tested at the same time. 
 */
static void lock_lock_func(void** state) {
	lock = 0;

	for(int i = 0; i < THREAD_NUMBER; i++) {
		pthread_create(&thds[i], NULL, &thread_lock, NULL);
	}
	for(int i = 0; i < THREAD_NUMBER; i++) {
		pthread_join(thds[i], NULL);
	}
	assert_int_equal(shared_data, 10000);
}

static void lock_trylock_func(void** state) {
	lock = 1;
	assert_false(lock_trylock(&lock));

	lock = 0;
	assert_true(lock_trylock(&lock));
}

static void lock_unlock_func(void** state) {
	lock = 1;
	lock_unlock(&lock);
	assert_int_equal(lock, 0);
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(lock_init_func),
		cmocka_unit_test(lock_lock_func),
		cmocka_unit_test(lock_trylock_func),
		cmocka_unit_test(lock_unlock_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}

void* thread_lock(void* arg) {
	lock_lock(&lock);
	
	// Critical section.
	for(int i = 0; i < 100; i++)
		shared_data++;

	lock_unlock(&lock);

	return NULL;
}
