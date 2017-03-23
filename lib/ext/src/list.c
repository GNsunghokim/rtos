#include <stddef.h>
#include <_malloc.h>
#include <util/list.h>

List* list_create(void* pool) {
	List* list = __malloc(sizeof(List), pool);
	if(!list)
		return NULL;
	
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	list->pool = pool;
	
	return list;
}

void list_destroy(List* list) {
	ListIterator iter;
	list_iterator_init(&iter, list);

	while(list_iterator_has_next(&iter)) {
		ListNode* node = iter.node;
		list_iterator_next(&iter);
		__free(node, list->pool);
	}
	
	__free(list, list->pool);
}

bool list_is_empty(List* list) {
	return list->head == NULL;
}

static void _add(List* list, ListNode* prev, ListNode* node) {
	if(prev) {
		if(prev->next) {
			prev->next->prev = node;
			node->next = prev->next;
		} else {
			list->tail = node;
		}
		
		prev->next = node;
		node->prev = prev;
	} else {
		list->head = list->tail = node;
	}
	
	list->size++;
}

bool list_add(List* list, void* data) {
	ListNode* node = __malloc(sizeof(ListNode), list->pool);
	if(!node)
		return false;
	
	node->data = data;
	node->prev = NULL;
	node->next = NULL;
	
	_add(list, list->tail, node);
	
	return true;
}

bool list_add_at(List* list, size_t index, void* data) {
	ListNode* node2 = __malloc(sizeof(ListNode), list->pool);
	if(!node2)
		return false;
	
	node2->data = data;
	node2->prev = NULL;
	node2->next = NULL;
	
	if(index <= 0) {
		if(list->head) {
			list->head->prev = node2;
			node2->next = list->head;
			list->head = node2;
		} else {
			list->head = list->tail = node2;
		}
		list->size++;
	} else if(index >= list->size) {
		_add(list, list->tail, node2);
	} else {
		index--;
		
		ListNode* node = list->head;
		while(index > 0) {
			if(!node->next)
				return false;
			
			node = node->next;
			index--;
		}
		
		_add(list, node, node2);
	}
	
	return true;
}

void* list_get(List* list, size_t index) {
	ListNode* node = list->head;
	while(index > 0) {
		if(!node->next)
			return NULL;
		
		node = node->next;
		index--;
	}
	
	if(node)
		return node->data;
	else
		return NULL;
}

void* list_get_first(List* list) {
	if(list->head)
		return list->head->data;
	else
		return NULL;
}

void* list_get_last(List* list) {
	if(list->tail)
		return list->tail->data;
	else
		return NULL;
}

static bool default_comp_fn(void* v1, void* v2) {
	return v1 == v2;
}

int list_index_of(List* list, void* data, bool(*comp_fn)(void*,void*)) {
	if(!comp_fn)
		comp_fn = default_comp_fn;
	
	int index = 0;
	ListIterator iter;
	list_iterator_init(&iter, list);
	while(list_iterator_has_next(&iter)) {
		void* data2 = list_iterator_next(&iter);
		if(comp_fn(data, data2))
			return index;
		
		index++;
	}
	
	return -1;
}

static void* _remove(List* list, ListNode* node) {
	if(list->head == node)
		list->head = node->next;
	
	if(list->tail == node)
		list->tail = node->prev;
	
	list->size--;
	
	if(node->prev)
		node->prev->next = node->next;
	
	if(node->next)
		node->next->prev = node->prev;
	
	void* data = node->data;
	__free(node, list->pool);
	
	return data;
}

void* list_remove(List* list, size_t index) {
	ListNode* node = list->head;

	while(index && node) {
		node = node->next;
		index--;
	}
	
	if(node)
		return _remove(list, node);
	else
		return NULL;
}

bool list_remove_data(List* list, void* data) {
	ListNode* node = list->head;
	while(node) {
		if(node->data == data) {
			_remove(list, node);
			return true;
		}
		
		node = node->next;
	}
	return false;
}

void* list_remove_first(List* list) {
	if(list->head == NULL)
		return NULL;

	return _remove(list, list->head);
}

void* list_remove_last(List* list) {
	if(list->tail == NULL)
		return NULL;

	return _remove(list, list->tail);
}

size_t list_size(List* list) {
	return list->size;
}

void list_rotate(List* list) {
	if(list->head != list->tail) {
		ListNode* node = list->head;
		
		list->head = node->next;
		list->head->prev = NULL;
		
		list->tail->next = node;
		node->prev = list->tail;
		list->tail = node;
	}
}

void list_iterator_init(ListIterator* iter, List* list) {
	iter->list = list;
	iter->prev = NULL;
	iter->node = list->head;
}

bool list_iterator_has_next(ListIterator* iter) {
	return iter->node != NULL;
}

void* list_iterator_next(ListIterator* iter) {
	if(iter->node) {
		void* data = iter->node->data;
		iter->prev = iter->node;
		iter->node = iter->node->next;
		
		return data;
	} else {
		return NULL;
	}
}

void* list_iterator_remove(ListIterator* iter) {
	if(iter->prev) {
		return _remove(iter->list, iter->prev);
	} else {
		return NULL;
	}
}
