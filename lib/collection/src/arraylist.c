#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <util/arraylist.h>

static bool grow(ArrayList* this) {
	size_t new_capacity = (this->capacity * 1.5) + 1;
	void** array = this->realloc(this->array, sizeof(void*) * new_capacity);
	if (!array)
		return false;

	this->array = array;
	this->capacity = new_capacity;
	return true;
}

static bool add(ArrayList* this, void* element) {
	if(this->size >= this->capacity && !grow(this))
		return false;

	this->array[this->size++] = element;
	return true;
}

static bool remove(ArrayList* this, void* element) {
	int index = this->index_of(this, element);
	if(index < 0)
		return false;

	this->size -= 1;
	memmove(&this->array[index], &this->array[index + 1], (this->size - index) * sizeof(void*));

	return true;
}

static void* get(ArrayList* this, size_t index) {
	if(index >= this->size)
		return NULL;
	else
		return this->array[index];
}

static void* set(ArrayList* this, size_t index, void* element) {
	if(index >= this->size)
		return NULL;

	void* old = this->array[index];
	this->array[index] = element;
	return old;
}

static void* remove_at(ArrayList* this, size_t index) {
	if(index >= this->size)
		return NULL;

	void* data = this->array[index];
	this->size--;
	memmove(&this->array[index], &this->array[index + 1], (this->size - index) * sizeof(void*));

	return data;
}

static bool add_at(ArrayList* this, size_t index, void* element) {
	if(this->size + 1 >= this->capacity && !grow(this))
		return false;
	if(index >= this->size)
		return false;

	memmove(&this->array[index + 1], &this->array[index], (this->size - index) * sizeof(void*));
	this->array[index] = element;
	this->size++;

	return true;
}

static int index_of(ArrayList* this, void* element) {
	for(size_t index = 0; index < this->size; index++)
		if(this->equals(this->array[index], element))
			return index;

	return -1;
}

static bool is_available(ArrayList* this) {
	return this->size < this->capacity;
}

static void iterator_init(ArrayListIterContext* context, ArrayList* list) {
	context->list	= list;
	context->index	= 0;
}

static bool iterator_has_next(ArrayListIterContext* context) {
	return context->index < context->list->size;
}

static void* iterator_next(ArrayListIterContext* context) {
	if(context->index < context->list->size)
		return context->list->array[context->index++];
	else
		return NULL;
}

static void* iterator_remove(ArrayListIterContext* context) {
	if(--context->index < context->list->size)
		return remove_at(context->list, context->index);
	else
		return NULL;
}

static Iterator iterator = {
	.init		= (void*)iterator_init,
	.has_next	= (void*)iterator_has_next,
	.next		= (void*)iterator_next,
	.remove		= (void*)iterator_remove,
};

ArrayList* arraylist_create(DataType type, PoolType pool, size_t initial_capacity) {
	if(!initial_capacity)
		return NULL;

	ArrayList* list = (ArrayList*)list_create(type, pool, sizeof(ArrayList));
	if(!list)
		return NULL;

	list->iter		= &iterator;
	list->capacity		= initial_capacity;
	list->array		= list->malloc(initial_capacity * sizeof(void*));
	if(!list->array) {
		list->free(list);
		return NULL;
	}

	list->add		= (void*)add;
	list->remove		= (void*)remove;
	list->get		= (void*)get;
	list->set		= (void*)set;
	list->remove_at 	= (void*)remove_at;
	list->add_at		= (void*)add_at;
	list->index_of		= (void*)index_of;
	list->is_available	= is_available;

	return list;
}

void arraylist_destroy(ArrayList* this) {
	void (*free)(void*) = this->free;
	free(this->array);

	list_destroy((List*)this);
}

