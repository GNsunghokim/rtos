#include <shared.h>

void** __shared;

void shared_set(void* ptr){
	*__shared = ptr;
}

void* shared_get(){
	return *__shared;
}
