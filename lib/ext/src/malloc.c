#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <_malloc.h>

extern void* __malloc_pool;

void* malloc(size_t size) {
	return __malloc(size, __malloc_pool);
}

void free(void *ptr) {
	__free(ptr, __malloc_pool);
}

void* realloc(void *ptr, size_t new_size) {
	return __realloc(ptr, new_size, __malloc_pool);
}

void* calloc(size_t nelem, size_t elem_size) {
	return __calloc(nelem, elem_size, __malloc_pool);
}

