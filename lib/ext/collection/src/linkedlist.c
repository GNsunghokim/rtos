#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <util/linkedlist.h>

static void _add(LinkedList* this, ListNode* prev, ListNode* node) {
	if(prev) {
		if(prev->next) {
			prev->next->prev = node;
			node->next = prev->next;
		} else {
			this->tail = node;
		}

		prev->next = node;
		node->prev = prev;
	} else {
		this->head = this->tail = node;
	}

	this->size++;
}

static bool add(LinkedList* this, void* element) {
	ListNode* node = this->malloc(sizeof(ListNode));
	if(!node)
		return false;

	node->data = element;
	node->prev = NULL;
	node->next = NULL;

	_add(this, this->tail, node);

	return true;
}

static void* _remove(LinkedList* this, ListNode* node) {
	if(this->head == node)
		this->head = node->next;

	if(this->tail == node)
		this->tail = node->prev;

	this->size--;

	if(node->prev)
		node->prev->next = node->next;

	if(node->next)
		node->next->prev = node->prev;

	void* data = node->data;
	this->free(node);

	return data;
}

static bool remove(LinkedList* this, void* element) {
	ListNode* node = this->head;
	while(node) {
		if(node->data == element) {
			_remove(this, node);
			return true;
		}

		node = node->next;
	}
	return false;
}

static void* get(LinkedList* this, size_t index) {
	ListNode* node = this->head;
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

static void* set(LinkedList* this, size_t index, void* element) {
	// Not yet implemented
	return NULL;
}

static void* remove_at(LinkedList* this, size_t index) {
	ListNode* node = this->head;

	while(index && node) {
		node = node->next;
		index--;
	}

	if(node)
		return _remove(this, node);
	else
		return NULL;
}

static bool add_at(LinkedList* this, size_t index, void* element) {
	ListNode* node2 = this->malloc(sizeof(ListNode));
	if(!node2)
		return false;

	node2->data = element;
	node2->prev = NULL;
	node2->next = NULL;

	if(index <= 0) {
		if(this->head) {
			this->head->prev = node2;
			node2->next = this->head;
			this->head = node2;
		} else {
			this->head = this->tail = node2;
		}
		this->size++;
	} else if(index >= this->size) {
		_add(this, this->tail, node2);
	} else {
		index--;

		ListNode* node = this->head;
		while(index > 0) {
			if(!node->next)
				return false;

			node = node->next;
			index--;
		}

		_add(this, node, node2);
	}

	return true;
}

static int index_of(LinkedList* this, void* element) {
	int index = 0;
	void* data;
	for_each(data, this) {
		if(this->compare(data, element))
			return index;
		index++;
	}

	return -1;
}

static bool add_first(LinkedList* this, void* element) {
	// Not yet implemented
	return false;
}

static bool add_last(LinkedList* this, void* element) {
	// Not yet implemented
	return false;
}

static void* remove_first(LinkedList* this) {
	if(this->head == NULL)
		return NULL;

	return _remove(this, this->head);
}

static void* remove_last(LinkedList* this) {
	if(this->tail == NULL)
		return NULL;

	return _remove(this, this->tail);
}

static void* get_first(LinkedList* this) {
	if(this->head)
		return this->head->data;
	else
		return NULL;
}

static void* get_last(LinkedList* this) {
	if(this->tail)
		return this->tail->data;
	else
		return NULL;
}

static void rotate(LinkedList* this) {
	if(this->head != this->tail) {
		ListNode* node = this->head;

		this->head = node->next;
		this->head->prev = NULL;

		this->tail->next = node;
		node->prev = this->tail;
		this->tail = node;
	}
}

static void iterator_init(LinkedListIterContext* context, LinkedList* list) {
	context->list	= list;
	context->prev 	= NULL;
	context->node 	= list->head;
}

static bool iterator_has_next(LinkedListIterContext* context) {
	return context->node != NULL;
}

static void* iterator_next(LinkedListIterContext* context) {
	if(context->node) {
		void* data = context->node->data;
		context->prev = context->node;
		context->node = context->node->next;

		return data;
	} else {
		return NULL;
	}
}

static void* iterator_remove(LinkedListIterContext* context) {
	if(context->prev) {
		return _remove(context->list, context->prev);
	} else {
		return NULL;
	}
}

static Iterator iterator = {
	.init		= (void*)iterator_init,
	.has_next	= (void*)iterator_has_next,
	.next		= (void*)iterator_next,
	.remove		= (void*)iterator_remove,
};

LinkedList* linkedlist_create(DataType type, PoolType pool) {
	LinkedList* list = (LinkedList*)list_create(type, pool, sizeof(LinkedList));
	if(!list)
		return NULL;

	list->iter		= &iterator;
	list->head		= NULL;
	list->tail		= NULL;

	list->add		= (void*)add;
	list->remove		= (void*)remove;
	list->get		= (void*)get;
	list->set		= (void*)set;
	list->remove_at 	= (void*)remove_at;
	list->add_at		= (void*)add_at;
	list->index_of		= (void*)index_of;

	list->add_first		= (void*)add_first;
	list->add_last		= (void*)add_last;
	list->remove_first	= (void*)remove_first;
	list->remove_last	= (void*)remove_last;
	list->get_first		= (void*)get_first;
	list->get_last		= (void*)get_last;

	list->rotate		= rotate;

	return list;
}

void linkedlist_destroy(LinkedList* this) {
	list_destroy((List*)this);
}

