#include <string.h>
#include <_malloc.h>
#include <util/set.h>

// TODO: Change accessing list using index to using iterator.

#define THRESHOLD(cap)	(((cap) >> 1) + ((cap) >> 2))	// 75%

Set* set_create(size_t initial_capacity, uint64_t(*hash)(void*), bool(*equals)(void*,void*), void* pool) {
	if(!equals)
		equals = set_uint64_equals;

	if(!hash)
		hash = set_uint64_hash;

	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;

	Set* set = __malloc(sizeof(Set), pool);
	if(!set)
		return NULL;

	set->table = __malloc(sizeof(List*) * capacity, pool);
	if(!set->table) {
		__free(set, pool);
		return NULL;
	}

	bzero(set->table, sizeof(List*) * capacity);
	set->capacity = capacity;
	set->threshold = THRESHOLD(capacity);
	set->size = 0;
	set->hash = hash;
	set->equals = equals;
	set->pool = pool;

	return set;
}

static void destroy(Set* set) {
	for(size_t i = 0; i < set->capacity; i++) {
		List* list = set->table[i];
		if(!list)
			continue;

		ListIterator iter;
		list_iterator_init(&iter, list);
		while(list_iterator_has_next(&iter)) {
			SetEntry* entry = list_iterator_next(&iter);
			__free(entry, set->pool);
		}

		list_destroy(list);
	}

	__free(set->table, set->pool);
}

void set_destroy(Set* set) {
	destroy(set);
	__free(set, set->pool);
}

bool set_is_empty(Set* set) {
	return set->size == 0;
}

bool set_put(Set* set, void* data) {
	if(set->size + 1 > set->threshold) {
		// Create new set
		Set set2;
		size_t capacity = set->capacity * 2;
		set2.table = __malloc(sizeof(List*) * capacity, set->pool);
		if(!set2.table)
			return false;
		bzero(set2.table, sizeof(List*) * capacity);
		set2.capacity = capacity;
		set2.threshold = THRESHOLD(capacity);
		set2.size = 0;
		set2.hash = set->hash;
		set2.equals = set->equals;
		set2.pool = set->pool;

		// Copy
		SetIterator iter;
		set_iterator_init(&iter, set);
		while(set_iterator_has_next(&iter)) {
			SetEntry* entry = set_iterator_next(&iter);
			if(!set_put(&set2, entry->data)) {
				destroy(&set2);
				return false;
			}
		}

		// Destory
		destroy(set);

		// Paste
		memcpy(set, &set2, sizeof(Set));
	}

	size_t index = set->hash(data) % set->capacity;
	if(!set->table[index]) {
		set->table[index] = list_create(set->pool);
		if(!set->table[index])
			return false;
	} else {
		size_t size = list_size(set->table[index]);
		for(size_t i = 0; i < size; i++) {
			SetEntry* entry = list_get(set->table[index], i);
			if(set->equals(entry->data, data))
				return false;
		}
	}

	SetEntry* entry = __malloc(sizeof(SetEntry), set->pool);
	if(!entry) {
		if(list_is_empty(set->table[index])) {
			list_destroy(set->table[index]);
			set->table[index] = NULL;
		}
		return false;
	}

	entry->data = data;
	entry->data = data;

	if(!list_add(set->table[index], entry)) {
		__free(entry, set->pool);
		if(list_is_empty(set->table[index])) {
			list_destroy(set->table[index]);
			set->table[index] = NULL;
		}

		return false;
	}
	set->size++;

	return true;
}

void* set_get(Set* set, void* data) {
	size_t index = set->hash(data) % set->capacity;
	if(!set->table[index]) {
		return NULL;
	}

	size_t size = list_size(set->table[index]);
	for(size_t i = 0; i < size; i++) {
		SetEntry* entry = list_get(set->table[index], i);
		if(set->equals(entry->data, data))
			return entry->data;
	}

	return NULL;
}

bool set_contains(Set* set, void* data) {
	size_t index = set->hash(data) % set->capacity;
	if(!set->table[index]) {
		return false;
	}

	size_t size = list_size(set->table[index]);
	for(size_t i = 0; i < size; i++) {
		SetEntry* entry = list_get(set->table[index], i);
		if(set->equals(entry->data, data))
			return true;
	}

	return false;
}

void* set_remove(Set* set, void* data) {
	size_t index = set->hash(data) % set->capacity;
	if(!set->table[index]) {
		return NULL;
	}

	size_t size = list_size(set->table[index]);
	for(size_t i = 0; i < size; i++) {
		SetEntry* entry = list_get(set->table[index], i);
		if(set->equals(entry->data, data)) {
			void* data = entry->data;
			list_remove(set->table[index], i);
			__free(entry, set->pool);

			if(list_is_empty(set->table[index])) {
				list_destroy(set->table[index]);
				set->table[index] = NULL;
			}

			set->size--;

			return data;
		}
	}

	return NULL;
}

size_t set_capacity(Set* set) {
	return set->capacity;
}

size_t set_size(Set* set) {
	return set->size;
}

void set_iterator_init(SetIterator* iter, Set* set) {
	iter->set = set;
	for(iter->index = 0; iter->index < set->capacity && !set->table[iter->index]; iter->index++);
	iter->list_index = 0;
}

bool set_iterator_has_next(SetIterator* iter) {
	if(iter->index >= iter->set->capacity)
		return false;

	if(iter->set->table[iter->index] && iter->list_index < list_size(iter->set->table[iter->index]))
		return true;

	for(iter->index++; iter->index < iter->set->capacity && !iter->set->table[iter->index]; iter->index++);

	if(iter->index < iter->set->capacity) {
		iter->list_index = 0;
		return true;
	} else {
		return false;
	}
}

SetEntry* set_iterator_next(SetIterator* iter) {
	SetEntry* entry = list_get(iter->set->table[iter->index], iter->list_index++);
	iter->entry.data = entry->data;
	iter->entry.data = entry->data;

	return &iter->entry;
}

SetEntry* set_iterator_remove(SetIterator* iter) {
	iter->list_index--;
	SetEntry* entry = list_remove(iter->set->table[iter->index], iter->list_index);
	iter->entry.data = entry->data;
	iter->entry.data = entry->data;
	__free(entry, iter->set->pool);

	if(list_is_empty(iter->set->table[iter->index])) {
		list_destroy(iter->set->table[iter->index]);
		iter->set->table[iter->index] = NULL;
	}

	iter->set->size--;

	return &iter->entry;
}

uint64_t set_uint64_hash(void* data) {
	return (uintptr_t)data;
}

bool set_uint64_equals(void* data1, void* data2) {
	return data1 == data2;
}

uint64_t set_string_hash(void* data) {
	char* c = data;
	uint32_t len = 0;
	uint32_t sum = 0;
	while(*c != '\0') {
		len++;
		sum += *c++;
	}

	return ((uint64_t)len) << 32 | (uint64_t)sum;
}

bool set_string_equals(void* data1, void* data2) {
	char* c1 = data1;
	char* c2 = data2;

	while(*c1 != '\0' && *c2 != '\0') {
		if(*c1++ != *c2++)
			return false;
	}

	if(*c1 != '\0' || *c2 != '\0')
		return false;

	return true;
}
