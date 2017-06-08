#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>
#include "gdt.h"
#include "mmap.h"

typedef struct {
	uint16_t limit;
	uint64_t base;
	uint16_t padding;
	uint32_t padding2;
} __attribute__ ((packed)) IDTR;

typedef struct {
	uint64_t offset: 16;	// Offset of procedure entry point
	uint64_t segment: 16;	// Segment selector
	uint64_t ist: 3;
	uint64_t zero: 5;
	uint64_t type: 4;	// 1111 = Trap, 1110 = Interrupt
	uint64_t s: 1;		// 0
	uint64_t dpl: 2;	// Descriptor privilege level
	uint64_t p: 1;		// Segment present
	uint64_t offset2: 48;
	uint64_t reserved: 32;
} __attribute__ ((packed)) ID;

#define IDTR_INIT(idtr)		GDTR_INIT(idtr)

#define ID_INIT(_id, _offset, _segment, _ist, _type, _dpl, _p) \
				(_id).offset = (_offset) & 0xffff; \
				(_id).offset2 = ((_offset) >> 16) & 0xffffffffffff; \
				(_id).segment = _segment; \
				(_id).ist = _ist; \
				(_id).zero = 0; \
				(_id).type = _type; \
				(_id).s = 0; \
				(_id).dpl = _dpl; \
				(_id).p = _p; \
				(_id).reserved = 0;

void idt_init();
void idt_load();

#endif /* __IDT_H__ */
