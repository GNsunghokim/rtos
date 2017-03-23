#include <tlsf.h>

void* __malloc_pool;

void* __malloc(size_t size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX
		return malloc(size);
#else
		return malloc_ex(size, __malloc_pool);
#endif
	} else 
		return malloc_ex(size, mem_pool);
}

void __free(void *ptr, void* mem_pool) { 
	if(!mem_pool) {
#ifdef LINUX
	return free(ptr);
#else
		free_ex(ptr, __malloc_pool);
#endif
	} else
		free_ex(ptr, mem_pool);
}

void* __realloc(void *ptr, size_t new_size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX
	return realloc(ptr, new_size);
#else
		return realloc_ex(ptr, new_size, __malloc_pool);
#endif
	} else
		return realloc_ex(ptr, new_size, mem_pool);
}

void* __calloc(size_t nelem, size_t elem_size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX
	return calloc(nelem, elem_size);
#else
		return calloc_ex(nelem, elem_size, __malloc_pool);
#endif
	} else
		return calloc_ex(nelem, elem_size, mem_pool);
}
