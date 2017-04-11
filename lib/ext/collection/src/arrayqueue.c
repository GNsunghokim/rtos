#include <util/arrayqueue.h>

static bool enqueue(ArrayQueue* this, void* element) {
	size_t next = (this->tail + 1) % this->capacity;
	if(this->head != next) {
		this->array[this->tail] = element;
		this->tail = next;
		this->size++;
		return true;
	}

	return false;
}

static void* dequeue(ArrayQueue* this) {
	if(this->head != this->tail) {
		void* data = this->array[this->head];
		this->head = (this->head + 1) % this->capacity;
		this->size--;
		return data;
	}

	return NULL;
}

static void* get(ArrayQueue* this, int index) {
	if(this->head != this->tail) {
		return this->array[(this->head + index) % this->capacity];
	}

	return NULL;
}

static void* peek(ArrayQueue* this) {
	return this->array[this->head];
}

static bool is_available(ArrayQueue* this) {
	return this->head != (this->tail + 1) % this->capacity;
}

static bool resize(ArrayQueue* this, size_t capacity, void (*popped)(void*)) {
	void** array = this->malloc(capacity * sizeof(void*));
	if(!array)
		return false;

	size_t tail = 0;
	while(!this->is_empty(this)) {
		if(tail < capacity - 1)
			array[tail++] = this->dequeue(this);
		else
			popped(this->dequeue(this));
	}

	this->free(this->array);

	this->capacity = capacity;
	this->array = array;
	this->head = 0;
	this->tail = tail;

	return true;
}

ArrayQueue* arrayqueue_create(DataType type, PoolType pool, size_t initial_capacity) {
	ArrayQueue* queue = (ArrayQueue*)queue_create(type, pool, sizeof(ArrayQueue));
	if(!queue)
		return NULL;

	queue->head		= 0;
	queue->tail		= 0;
	queue->capacity		= initial_capacity;
	queue->array		= queue->malloc(initial_capacity * sizeof(void*));
	if(!queue->array) {
		queue->free(queue);
		return NULL;
	}

	queue->enqueue		= queue->add = (void*)enqueue;
	queue->dequeue		= (void*)dequeue;
	queue->get		= (void*)get;
	queue->peek		= (void*)peek;
	queue->is_available	= is_available;
	queue->resize		= resize;

	return queue;
}

void arrayqueue_destroy(ArrayQueue* this) {
	void (*free)(void*) = this->free;
	free(this->array);

	queue_destroy((Queue*)this);
}

