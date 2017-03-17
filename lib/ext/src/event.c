#include <malloc.h>
#include <util/list.h>
#include <util/map.h>
#include <util/event.h>
#include <timer.h>

typedef struct {
	EventFunc	func;
	void*		context;
} Node;

typedef struct {
	EventFunc	func;
	void*		context;
	clock_t		delay;
	clock_t		period;
} TimerNode;

typedef struct {
	uint64_t		event_id;
	TriggerEventFunc	func;
	void*			context;
} TriggerNode;

typedef struct {
	uint64_t		event_id;
	void*			event;
	TriggerEventFunc	last;
	void*			last_context;
} Trigger;

static List* busy_events;
static List* timer_events;
static Map* trigger_events;
static List* triggers;
static List* idle_events;

void event_init() {
/*
 *#ifndef LINUX
 *        extern uint64_t __timer_ms;
 *        if(!__timer_ms)
 *                return;
 *#endif
 */

	busy_events = list_create(NULL);
	timer_events = list_create(NULL);
	trigger_events = map_create(8, map_uint64_hash, map_uint64_equals, NULL);
	triggers = list_create(NULL);
	idle_events = list_create(NULL);
}

static bool is_trigger_stop;

static void fire(uint64_t event_id, void* event, TriggerEventFunc last, void* last_context) {
	List* list = map_get(trigger_events, (void*)(uintptr_t)event_id);
	if(!list)
		goto done;
	
	ListIterator iter;
	list_iterator_init(&iter, list);
	while(list_iterator_has_next(&iter)) {
		TriggerNode* node = list_iterator_next(&iter);
		is_trigger_stop = false;
		if(!node->func(event_id, event, node->context)) {
			list_iterator_remove(&iter);
			free(node);
		}
		
		if(is_trigger_stop)
			return;
	}
	
done:
	if(last)
		last(event_id, event, last_context);
}

static bool get_first_bigger(void* time, void* node) {
	return (clock_t)time < ((TimerNode*)node)->delay;
}

static uint64_t next_timer = UINT64_MAX;

int event_loop() {
	int count = 0;
	
	// Busy events
	ListIterator iter;
	list_iterator_init(&iter, busy_events);
	while(list_iterator_has_next(&iter)) {
		Node* node = list_iterator_next(&iter);
		if(!node->func(node->context)) {
			list_iterator_remove(&iter);
			free(node);
		}
	}
	
	// Trigger events
	while(list_size(triggers) > 0) {
		Trigger* trigger = list_remove_first(triggers);
		fire(trigger->event_id, trigger->event, trigger->last, trigger->last_context);
		free(trigger);
		
		count++;
	}
	
	if(count > 0)
		return count;
	
	// Timer events
	uint64_t time = timer_us();
	while(next_timer <= time) {
		TimerNode* node = list_remove_first(timer_events);
		if(node->func(node->context)) {
			node->delay += node->period;
			int index = list_index_of(timer_events, (void*)(uintptr_t)node->delay, get_first_bigger);
			if(index == -1) {
				if(!list_add(timer_events, node)) {
					free(node);

					printf("Timer event lost unexpectedly cause of memory lack!!!\n");
					while(1) __asm__ __volatile__  ("hlt");
				}
			} else {
				if(!list_add_at(timer_events, index, node)) {
					free(node);

					printf("Timer event lost unexpectedly cause of memory lack!!!\n");
					while(1) __asm__ __volatile__  ("hlt");
				}
			}
		} else {
			free(node);
		}
		
		if(list_size(timer_events) > 0)
			next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
		else
			next_timer = INT64_MAX;
		
		count++;
	}

	if(count > 0)
		return count;
	
	// Idle events
	if(list_size(idle_events) > 0) {
		Node* node = list_get_first(idle_events);
		if(!node->func(node->context)) {
			list_remove_first(idle_events);
			free(node);
		}
		
		list_rotate(idle_events);
		
		count++;
	}
	
	return count;
}

uint64_t event_busy_add(EventFunc func, void* context) {
	Node* node = malloc(sizeof(Node));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	
	if(!list_add(busy_events, node)) {
		free(node);
		return 0;
	}
	
	return (uintptr_t)node;
}

bool event_busy_remove(uint64_t id) {
	if(list_remove_data(busy_events, (void*)(uintptr_t)id)) {
		free((void*)(uintptr_t)id);
		return true;
	} else {
		return false;
	}
}

uint64_t event_timer_add(EventFunc func, void* context, clock_t delay, clock_t period) {
	TimerNode* node = malloc(sizeof(TimerNode));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	uint64_t time = timer_us();

	node->delay = time + delay;
	node->period = period;
	
	int index = list_index_of(timer_events, (void*)(uintptr_t)node->delay, get_first_bigger);
	if(index == -1) {
		if(!list_add(timer_events, node)) {
			free(node);
			return 0;
		}
	} else {
		if(!list_add_at(timer_events, index, node)) {
			free(node);
			return 0;
		}
	}
	
	next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
	
	return (uintptr_t)node;
}

bool event_timer_update(uint64_t id, clock_t period) {
	if(list_remove_data(timer_events, (void*)id)) {
		TimerNode* node = (TimerNode*)id;
		uint64_t time = timer_us();
		node->period = period;
		node->delay = time + node->period;
		
		int index = list_index_of(timer_events, (void*)(uint64_t)node->delay, get_first_bigger);
		if(index == -1) {
			if(!list_add(timer_events, node)) {
				free(node);
				return false;
			}
		} else {
			if(!list_add_at(timer_events, index, node)) {
				free(node);
				return false;
			}
		}

		next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
		
		return true;
	} else {
		return false;
	}
}

bool event_timer_remove(uint64_t id) {
	if(list_remove_data(timer_events, (void*)(uintptr_t)id)) {
		free((void*)(uintptr_t)id);
		
		if(list_size(timer_events) > 0)
			next_timer = ((TimerNode*)list_get_first(timer_events))->delay;
		else
			next_timer = INT64_MAX;
		
		return true;
	} else {
		return false;
	}
}

uint64_t event_trigger_add(uint64_t event_id, TriggerEventFunc func, void* context) {
	TriggerNode* node = malloc(sizeof(TriggerNode));
	if(!node)
		return 0;
	node->event_id = event_id;
	node->func = func;
	node->context = context;
	
	List* list = map_get(trigger_events, (void*)(uintptr_t)event_id);
	if(!list) {
		list = list_create(NULL);
		if(!list) {
			free(node);
			return 0;
		}
		
		map_put(trigger_events, (void*)(uintptr_t)event_id, list);
	}
	
	list_add(list, node);
	
	return (uintptr_t)node;
}

bool event_trigger_remove(uint64_t id) {
	MapIterator iter;
	map_iterator_init(&iter, trigger_events);
	while(map_iterator_has_next(&iter)) {
		List* list = map_iterator_next(&iter)->data;
		if(list_remove_data(list, (void*)(uintptr_t)id)) {
			free((void*)(uintptr_t)id);
			
			if(list_size(list) == 0) {
				map_iterator_remove(&iter);
				list_destroy(list);
			}
			
			return true;
		}
	}
	
	return false;
}

void event_trigger_fire(uint64_t event_id, void* event, TriggerEventFunc last, void* last_context) {
	Trigger* trigger = malloc(sizeof(Trigger));
	if(!trigger) {
		fire(event_id, event, last, last_context);
		return;
	}
	trigger->event_id = event_id;
	trigger->event = event;
	trigger->last = last;
	trigger->last_context = last_context;
	
	if(!list_add(triggers, trigger)) {
		free(trigger);
		fire(event_id, event, last, last_context);
	}
}

void event_trigger_stop() {
	is_trigger_stop = true;
}

uint64_t event_idle_add(EventFunc func, void* context) {
	Node* node = malloc(sizeof(Node));
	if(!node)
		return 0;
	node->func = func;
	node->context = context;
	
	if(!list_add(idle_events, node)) {
		free(node);
		return 0;
	}
	
	return (uintptr_t)node;
}

bool event_idle_remove(uint64_t id) {
	if(list_remove_data(idle_events, (void*)(uintptr_t)id)) {
		free((void*)(uintptr_t)id);
		return true;
	} else {
		return false;
	}
}
