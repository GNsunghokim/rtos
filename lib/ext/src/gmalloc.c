#include <stdint.h>
#include <thread.h>
#include <_malloc.h>
#include <gmalloc.h>

/* These functions could be replaced with kernel implemtation with strong symbols */
void* __gmalloc_pool;

void* __attribute__((weak)) gmalloc(size_t size) {
	return __malloc(size, __gmalloc_pool);
}

void __attribute__((weak)) gfree(void *ptr) {
	__free(ptr, __gmalloc_pool);
}

void* __attribute__((weak)) gcalloc(size_t nmemb, size_t size) {
	return __calloc(nmemb, size, __gmalloc_pool);
}

void* __attribute__((weak)) grealloc(void *ptr, size_t size) {
	return __realloc(ptr, size, __gmalloc_pool);
}
