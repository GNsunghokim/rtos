#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "util/cache.h"

#define CMOCKA_MESSAGE_OUTPUT XML
#define CMOCKA_XML_FILE output.XML

/* A test case that does nothing and succeeds. */
static void null_test_success(void **state) {
	(void) state; /* unused */
	
	Cache* cache = cache_create(1000, NULL, NULL);
	for(int i = 0; i < 1001; i++) {
		assert_true(cache_set(cache, (void*)(uintptr_t)i, "hello world"));
	}
	
}
int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(null_test_success),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}

#if 0
int main() {
	void show(int size, Cache* cache) {
		for(int i = 0; i < size; i++) {
			if(!cache_get(cache, (void*)(uintptr_t)i))
				printf("%3d(X)\t", i);
			else
				printf("%3d   \t", i);
		}
		printf("\n");
	}

	Cache* cache;
	int num = 4;

	switch(num) {
		case 0: 
			// Charcater constant free
			break;
		case 1:
			// Malloc free
			cache = cache_create(1000, free, NULL);
			for(int i = 0; i < 4001; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			break;
		case 2:
			// Key update check
			cache = cache_create(1000, free, NULL);
			for(int i = 0; i < 1500; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			for(int i = 0; i < 1000; i++) {
				if(!cache_get(cache, (void*)(uintptr_t)i))
					printf("%d(X)\n", i);
				else
					printf("%d\n", i);
			}

			break;
		case 3: 
			// Cash clear  
			cache = cache_create(200, free, NULL);
			for(int i = 0; i < 200; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			printf("Set after\n");
			for(int i = 0; i < 200; i++) {
				if(!cache_get(cache, (void*)(uintptr_t)i))
					printf("%3d(X)\t", i);
				else
					printf("%3d   \t", i);
			}
			show(200, cache);

			cache_clear(cache);
			printf("Clear after\n");
			show(200, cache);

			for(int i = 0; i < 300; i++) {
				char* buf = malloc(10000);
				if(!cache_set(cache, (void*)(uintptr_t)i, buf)) {
					printf("%d fail\n", i);
					return -1;
				}
			}
			printf("Set again after\n");
			show(200, cache);
		case 4: 
			// Destroy & create
			for(int i = 0; i < 1000000; i++) {
				cache = cache_create(200, free, NULL);
				cache_destroy(cache);
			}
			
	}

	return 0;
}
#endif
