#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

uint64_t TIMER_FREQUENCY_PER_SEC;
uint64_t __timer_ms;
uint64_t __timer_us;
uint64_t __timer_ns;

uint64_t timer_frequency() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));
	
	return time;
}

void timer_init(const char* cpu_brand) {
	// e.g. Intel(R) Core(TM) i7-3770 CPU @ 3.40GHz
#ifdef LINUX
	/* Linux don't need to calculate frequency */
	return;
#endif
	if(strstr(cpu_brand, "Intel") != NULL && strstr(cpu_brand, "@ ") != NULL) {
		int number = 0;
		int is_dot_found = 0;
		int dot = 0;

		const char* ch = strstr(cpu_brand, "@ ");
		ch += 2;
		while((*ch >= '0' && *ch <= '9') || *ch == '.') {
			if(*ch == '.') {
				is_dot_found = 1;
			} else {
				number *= 10;
				number += *ch - '0';

				dot += is_dot_found;
			}

			ch++;
		}

		uint64_t frequency = 0;
		if(strncmp(ch, "THz", 3) == 0) {
		} else if(strncmp(ch, "GHz", 3) == 0) {
			frequency = 1000000000L;
		} else if(strncmp(ch, "MHz", 3) == 0) {
			frequency = 1000000L;
		} else if(strncmp(ch, "KHz", 3) == 0) {
			frequency = 1000L;
		} else if(strncmp(ch, "Hz", 3) == 0) {
			frequency = 1L;
		}

		while(dot > 0) {
			frequency /= 10;
			dot--;
		}
		TIMER_FREQUENCY_PER_SEC = frequency * number;
	} else {
		#define PIT_CONTROL     0x43
		#define PIT_COUNTER0    0x40
		#define PIT_FREQUENCY	1193182
		
		void measure(uint64_t* tsc, uint16_t* pit, uint64_t loop) {
			uint64_t base_tsc = timer_frequency();
			
			uint8_t pit0, pit1;
			asm volatile ("inb %%dx,%%al" : "=a"(pit0) : "d"(PIT_COUNTER0));
			asm volatile ("inb %%dx,%%al" : "=a"(pit1) : "d"(PIT_COUNTER0));
			uint16_t base_pit = (uint16_t)pit0 | (uint16_t)pit1 << 8;
			
			for(uint64_t i = 0; i < loop; i++) {
				asm("nop");
			}
			
			uint64_t time_tsc = timer_frequency();
			
			asm volatile ("inb %%dx,%%al" : "=a"(pit0) : "d"(PIT_COUNTER0));
			asm volatile ("inb %%dx,%%al" : "=a"(pit1) : "d"(PIT_COUNTER0));
			uint16_t time_pit = (uint16_t)pit0 | (uint16_t)pit1 << 8;
			
			*tsc = time_tsc - base_tsc;
			*pit = time_pit > base_pit ? 0xffff - (time_pit - base_pit) : base_pit - time_pit;
		}
		
		uint16_t target = PIT_FREQUENCY / 100;
		
		uint64_t last_tsc = 0;
		//uint32_t last_loop = 0;
		uint16_t last_d = 0xffff;
		
		int retry = 0;
		uint32_t u = 0;
		uint64_t loop = 100000;
		for(int i = 0; i < 30; i++) {
			uint64_t tsc;
			uint16_t pit;
			measure(&tsc, &pit, loop);
			
			uint16_t d = pit < target ? target - pit : pit - target;
			if(d < last_d) {
				last_tsc = tsc;
				//last_loop = loop;
				last_d = d;
			} else if(d == 0) {
				break;
			}
			
			int scale = target / pit;
			if(scale > 1) {
				loop *= scale;
			} else {
				if(u == 0) {
					u = loop / 3;
				} else if(u == 1) {
					if(++retry >= 3)
						break;
				} else {
					u /= 2;
					if(u <= 0)
						u = 1;
				}
				
				if(pit < target) {
					loop += u;
				} else {
					loop -= u;
				}
			}
		}
		
		TIMER_FREQUENCY_PER_SEC = last_tsc * 100;
	}
	
	__timer_ms = TIMER_FREQUENCY_PER_SEC / 1000;
	__timer_us = __timer_ms / 1000;
	__timer_ns = __timer_us / 1000;
}

void timer_swait(uint32_t s) {
	uint64_t time = timer_frequency();
	time += TIMER_FREQUENCY_PER_SEC * s;
	while(timer_frequency() < time)
		asm volatile("nop");
}

void timer_mwait(uint32_t ms) {
	uint64_t time = timer_frequency();
	time += __timer_ms * ms;
	while(timer_frequency() < time)
		asm volatile("nop");
}

void timer_uwait(uint32_t us) {
	uint64_t time = timer_frequency();
	time += __timer_us * us;
	while(timer_frequency() < time)
		asm volatile("nop");
}

void timer_nwait(uint32_t ns) {
	uint64_t time = timer_frequency();
	time += __timer_ns * ns;
	while(timer_frequency() < time)
		asm volatile("nop");
}

uint64_t timer_ns() {
#ifdef LINUX
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (uint64_t)-1;

	return (uint64_t)(ts.tv_nsec + ts.tv_sec * 1000000000);

#else /* LINUX */
	/* (Current CPU Time Stamp Counter / CPU frequency per nano-seconds) */
	return (uint64_t)(timer_frequency() / __timer_ns); 
#endif
}

uint64_t timer_us() {
#ifdef LINUX
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (uint64_t)-1;

	return (uint64_t)(ts.tv_nsec / 1000 + ts.tv_sec * 1000000);

#else /* LINUX */
	/* (Current CPU Time Stamp Counter / CPU frequency per nano-seconds) */
	return (uint64_t)(timer_frequency() / __timer_us);
#endif
}

uint64_t timer_ms() {
#ifdef LINUX
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (uint64_t)-1;

	return (uint64_t)(ts.tv_nsec / 1000000 + ts.tv_sec * 1000);

#else /* LINUX */
	/* (Current CPU Time Stamp Counter / CPU frequency per nano-seconds) */
	return (uint64_t)(timer_frequency() / __timer_ms);
#endif
}

uint64_t timer_s() {
#ifdef LINUX
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (uint64_t)-1;

	return (uint64_t)(ts.tv_sec);

#else /* LINUX */
	/* (Current CPU Time Stamp Counter / CPU frequency per nano-seconds) */
	return (uint64_t)(timer_frequency() / TIMER_FREQUENCY_PER_SEC);
#endif
}

