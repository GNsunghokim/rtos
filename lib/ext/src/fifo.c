#include <stddef.h>
#include <_malloc.h>
#include <util/fifo.h>

FIFO* fifo_create(size_t size, void* pool) {
	FIFO* fifo = __malloc(sizeof(FIFO), pool);
	if(!fifo)
		return NULL;
	
	void* array = __malloc(size * sizeof(void*), pool);
	if(!array) {
		__free(fifo, pool);
		return NULL;
	}
	
	fifo_init(fifo, array, size);
	fifo->pool = pool;
	
	return fifo;
}

void fifo_destroy(FIFO* fifo) {
	__free(fifo->array, fifo->pool);
	__free(fifo, fifo->pool);
}

bool fifo_resize(FIFO* fifo, size_t size, void(*popped)(void*)) {
	void* array = __malloc(size * sizeof(void*), fifo->pool);
	if(!array)
		return false;
	void* _array = fifo->array;
	fifo_reinit(fifo, array, size, popped);
	__free(_array, fifo->pool);
	
	return true;
}

void fifo_init(FIFO* fifo, void** array, size_t size) {
	fifo->head = 0;
	fifo->tail = 0;
	fifo->size = size;
	fifo->array = array;
	fifo->pool = NULL;
}

void fifo_reinit(FIFO* fifo, void** array, size_t size, void(*popped)(void*)) {
	size_t tail = 0;
	while(!fifo_empty(fifo)) {
		if(tail < size - 1) 
			array[tail++] = fifo_pop(fifo);
		else {
			popped(fifo_pop(fifo));
		}
	}

	fifo->size = size;
	fifo->array = array;
	fifo->head = 0;
	fifo->tail = tail;
}

bool fifo_push(FIFO* fifo, void* data) {
	size_t next = (fifo->tail + 1) % fifo->size;
	if(fifo->head != next) {
		fifo->array[fifo->tail] = data;
		fifo->tail = next;
		
		return true;
	} else {
		return false;
	}
}

void* fifo_pop(FIFO* fifo) {
	if(fifo->head != fifo->tail) {
		void* data = fifo->array[fifo->head];
		fifo->head = (fifo->head + 1) % fifo->size;
		
		return data;
	} else {
		return NULL;
	}
}

void* fifo_peek(FIFO* fifo, size_t index) {
	if(fifo->head != fifo->tail) {
		return fifo->array[(fifo->head + index) % fifo->size];
	} else {
		return NULL;
	}
}

size_t fifo_size(FIFO* fifo) {
	if(fifo->tail >= fifo->head)
		return fifo->tail - fifo->head;
	else
		return fifo->size + fifo->tail - fifo->head;
}

size_t fifo_capacity(FIFO* fifo) {
	return fifo->size - 1;
}

bool fifo_available(FIFO* fifo) {
	return fifo->head != (fifo->tail + 1) % fifo->size;
}

bool fifo_empty(FIFO* fifo) {
	return fifo->head == fifo->tail;
}
