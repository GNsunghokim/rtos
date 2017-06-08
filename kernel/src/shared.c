#include <stdio.h>
#include <string.h>
#include "mmap.h"
#include "asm.h"
#include "shared.h"
#include "page.h"

void shared_init() {
	Shared* shared = (Shared*)VIRTUAL_TO_PHYSICAL(SHARED_ADDR);
	uint8_t apic_id = mp_apic_id();
	if(apic_id == 0 /*BSP*/) {
		//printf("\tShared space: %p\n", VIRTUAL_TO_PHYSICAL(SHARED_ADDR));
	}

	if(shared->magic != SHARED_MAGIC) {
		//printf("\tWrong Magic Number: %lx\n", shared->magic);
		hlt();
	}
}

void shared_sync() {
	Shared* shared = (Shared*)VIRTUAL_TO_PHYSICAL(SHARED_ADDR);
	static uint8_t barrier;
	uint8_t apic_id = mp_apic_id();
	if(apic_id) {
		while(shared->sync <= barrier)
			asm volatile("nop");

		barrier++;
	} else {
		shared->sync = ++barrier;
	}
}
