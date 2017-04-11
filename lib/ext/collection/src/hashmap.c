#include <string.h>
#include <util/hashmap.h>

#define HASHMAP_THRESHOLD(cap)	(((cap) >> 1) + ((cap) >> 2))	// 75%

static void _destroy(HashMap* this) {
	for(size_t i = 0; i < this->capacity; i++) {
		LinkedList* list = this->table[i];
		if(!list)
			continue;

		Iterator* iter = list->iter;
		LinkedListIterContext context;
		iter->init(&context, list);
		while(iter->has_next(&context)) {
			MapEntry* entry = iter->next(&context);
			list->free(entry);
		}

		linkedlist_destroy(list);
	}
}

static void* get(HashMap* this, void* key) {
	size_t index = this->hash(key) % this->capacity;
	LinkedList* list = this->table[index];
	if(!list)
		return NULL;

	size_t size = list->size;
	for(size_t i = 0; i < size; i++) {
		MapEntry* entry = list->get(list, i);
		if(this->equals(entry->key, key))
			return entry->data;
	}

	return NULL;
}

static bool put(HashMap* this, void* key, void* value) {
	if(this->size + 1 > this->threshold) {
		// Create new this
		HashMap this2;
		size_t capacity = this->capacity * 2;
		this2.table = this->malloc(sizeof(LinkedList*) * capacity);
		if(!this2.table)
			return false;

		memset(this2.table, 0x0, sizeof(LinkedList*) * capacity);

		this2.capacity = capacity;
		this2.threshold = HASHMAP_THRESHOLD(capacity);
		this2.size = 0;

		// Copy
		/*
		 *MapIterator iter;
		 *this_iterator_init(&iter, this);
		 *while(this_iterator_has_next(&iter)) {
		 *        MapEntry* entry = this_iterator_next(&iter);
		 *        if(!this_put(&this2, entry->key, entry->data)) {
		 *                destroy(&this2);
		 *                return false;
		 *        }
		 *}
		 */

		// Destory
		_destroy(this);

		// Paste
		memcpy(this, &this2, sizeof(HashMap));
	}

	size_t index = this->hash(key) % this->capacity;
	LinkedList* list = this->table[index];
	if(!list) {
		list = linkedlist_create(this->type, this->pool);
		if(!list)
			return false;
	} else {
		size_t size = list->size;
		for(size_t i = 0; i < size; i++) {
			MapEntry* entry = list->get(list, i);
			if(this->equals(entry->key, key))
				return false;
		}
	}

	MapEntry* entry = this->malloc(sizeof(MapEntry));
	if(!entry) {
		if(list->is_empty(list))
			linkedlist_destroy(list);

		return false;
	}

	entry->key = key;
	entry->data = value;

	if(!list->add(list, entry)) {
		this->free(entry);

		if(list->is_empty(list))
			linkedlist_destroy(list);

		return false;
	}
	this->size++;

	return true;
}

static bool update(HashMap* this, void* key, void* value) {
	size_t index = this->hash(key) % this->capacity;
	LinkedList* list = this->table[index];
	if(!list)
		return false;

	size_t size = list->size;
	for(size_t i = 0; i < size; i++) {
		MapEntry* entry = list->get(list, i);
		if(this->equals(entry->key, key)) {
			entry->data = value;
			return true;
		}
	}

	return false;
}

static void* remove(HashMap* this, void* key) {
	size_t index = this->hash(key) % this->capacity;
	LinkedList* list = this->table[index];
	if(!list)
		return NULL;

	size_t size = list->size;
	for(size_t i = 0; i < size; i++) {
		MapEntry* entry = list->get(list, i);
		if(this->equals(entry->key, key)) {
			void* data = entry->data;
			list->remove_at(list, i);
			this->free(entry);

			if(list->is_empty(list))
				linkedlist_destroy(list);

			this->size--;

			return data;
		}
	}

	return NULL;
}

static bool contains_key(HashMap* this, void* key) {
	return get(this, key) != NULL;
}

static bool contains_value(HashMap* this, void* value) {
	// Not implemented yet
	return false;
}

typedef struct _EntrySetIterContext {
	HashMap*		_map;
	size_t			index;
	size_t			list_index;
	MapEntry		entry;
} EntrySetIterContext;

typedef struct _EntrySet {
	Set;

	HashMap*		map;
	EntrySetIterContext*	context;
} EntrySet;

typedef struct _KeySet {
	EntrySet;
} KeySet;

typedef struct _ValueSet {
	EntrySet;
} ValueSet;

static Set* entries(void* this) {
	return ((HashMap*)this)->entry_set;
}

static Set* keys(void* this) {
	return ((HashMap*)this)->key_set;
}

static Set* values(void* this) {
	return ((HashMap*)this)->value_set;
}

static void entry_iterator_init(EntrySetIterContext* context, EntrySet* entry_set) {
	HashMap* map = entry_set->map;
	context->_map = map;
	context->index = 0;
	while(context->index < map->capacity && map->table[context->index])
		context->index++;

	context->list_index = 0;
}

static bool entry_iterator_has_next(EntrySetIterContext* context) {
	HashMap* map = context->_map;
	if(context->index >= map->capacity)
		return false;

	LinkedList* list = map->table[context->index];
	if(list && context->list_index < list->size(list))
		return true;

	while(context->index < map->capacity && !map->table[context->index])
		context->index++;

	if(context->index < map->capacity) {
		context->list_index = 0;
		return true;
	} else {
		return false;
	}
}

static void* entry_iterator_next(EntrySetIterContext* context) {
	HashMap* map = context->_map;
	LinkedList* list = map->table[context->index];
	MapEntry* entry = list->get(list, context->list_index++);

	context->entry.key = entry->key;
	context->entry.data = entry->data;

	return &context->entry;
}

static void* entry_iterator_remove(EntrySetIterContext* context) {
	context->list_index--;
	
	LinkedList* list = iter->map->table[iter->index];
	MapEntry* entry = list->remove_at(list, context->list_index);
	context->entry.key = entry->key;
	context->entry.data = entry->data;
	free(entry, context->map->pool);
	
	if(list_is_empty(context->map->table[context->index])) {
		list_destroy(context->map->table[context->index]);
		context->map->table[context->index] = NULL;
	}
	
	context->map->size--;
	
	return &context->entry;
}

static void* key_iterator_next(EntrySetIterContext* context) {
}

static void* key_iterator_next(EntrySetIterContext* context) {
}

static Iterator entry_iterator = {
	.init		= (void*)entry_iterator_init,
	.has_next	= (void*)entry_iterator_has_next,
	.next		= (void*)entry_iterator_next,
	.remove		= (void*)entry_iterator_remove,
};

static Iterator key_iterator = {
	.init		= (void*)entry_iterator_init,
	.has_next	= (void*)entry_iterator_has_next,
	.next		= (void*)value_iterator_next,
	.remove		= (void*)entry_iterator_remove,
};

static Iterator value_iterator = {
	.init		= (void*)entry_iterator_init,
	.has_next	= (void*)entry_iterator_has_next,
	.next		= (void*)key_iterator_next,
	.remove		= (void*)entry_iterator_remove,
};

HashMap* hashmap_create(DataType type, PoolType pool, int initial_capacity) {
	HashMap* map = (HashMap*)map_create(type, pool, sizeof(HashMap));
	if(!map)
		return NULL;

	size_t capacity = 1;
	while(capacity < initial_capacity)
		capacity <<= 1;

	map->table = map->malloc(sizeof(LinkedList*) * capacity);
	if(!map->table) {
		map->free(map);
		return NULL;
	}
	memset(map->table, 0x0, sizeof(LinkedList*) * capacity);

	map->capacity		= capacity;
	map->threshold		= HASHMAP_THRESHOLD(capacity);

	map->get		= (void*)get;
	map->put		= (void*)put;
	map->update		= (void*)update;
	map->remove		= (void*)remove;
	map->contains_key	= (void*)contains_key;
	map->contains_value	= (void*)contains_value;

	map->entry_set = set_create(DATATYPE_UINT64, pool, sizeof(EntrySet));
	if(!map->set) {
		map->free(map);
		return NULL;
	}
	map->entry_set->map	= map;
	map->entry_set->iter	= &iterator;

	map->value_set		= (void*)value_set;
	map->entry_set		= (void*)entry_set;
	map->keys		= (void*)keys;

	return map;
}

void hashmap_destroy(HashMap* this) {
	_destroy(this);
        map_destroy((Map*)this);
}
