#include <lock.h>
#include <thread.h>

int __thread_id;
int __thread_count;

uint8_t volatile* __barrior_lock;
uint32_t volatile* __barrior;

int thread_id() {
	return __thread_id;
}

int thread_count() {
	return __thread_count;
}

void thread_barrior() {
	uint32_t map = 1 << __thread_id;
	
	uint32_t full = 0;
	for(int i = 0; i < __thread_count; i++)
		full |= 1 << i;
	
	lock_lock(__barrior_lock);
	if(*__barrior == full) {	// The first one
		*__barrior = map;
	} else {
		*__barrior |= map;
	}
	lock_unlock(__barrior_lock);
	
	while(*__barrior != full && *__barrior & map)
		__asm__ __volatile__ ("nop");
}
