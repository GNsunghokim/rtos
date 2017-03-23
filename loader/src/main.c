#include "multiboot2.h"
#include "page.h"
#include "print.h"
#include "pnkc.h"
#include "symbols.h"
#include "time.h"
#include "apic.h"

/***
 * Error code list
 * multiboot2 	0x1?
 * cpuid 	0x2?
 * longmode	0x3?
 * gdt		0x4?
 * page table	0x5?
 * pae		0x6?
 * pml4		0x7?
 * longmode	0x8?
 * paging	0x9?
 * kernel	0x10?
 */
static uint32_t kernel_start;
static uint32_t kernel_end;
static uint32_t initrd_start;
static uint32_t initrd_end;
static uint32_t multiboot2_addr;

static void log(char* title) {
	print(title);
}

static void log_32(char* title, uint32_t value) {
	print(title);
	print_32(value);
}

static void log_pass() {
	tab(75);
	print_attr = PASS;
	print("OK\n");
	print_attr = NORMAL;
}

static void log_fail(uint32_t code, char* message) {
	tab(75);
	print_attr = FAIL;
	print("FAIL\n");
	print("    Error Code: ");
	print_32(code);
	print("    ");
	print(message);

	while(1)
		asm("hlt");
}

static void cpuid(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
	asm volatile("cpuid"
		: "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
		: "a"(*a), "c"(*c));
}

uint8_t get_apic_id() {
	uint32_t a = 0x01, b, c, d;
	cpuid(&a, &b, &c, &d);

	return (b >> 24) & 0xff;
}

void check_multiboot2(uint32_t magic, uint32_t addr) {
	log_32("Check multiboot2 bootloader: ", magic);
	if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		log_fail(0x10, "Illegal multiboot2 bootloader magic");
	} else {
		log_pass();
	}

	log_32("Check multiboot2 header: 0x", addr);
	if(addr == 0) {
		log_fail(0x11, "Cannot find multiboot2 header");
	} else {
		log_pass();
	}

	multiboot2_addr = addr;

	uint32_t end = addr + *(uint32_t*)addr;
	addr += 8;

	while(addr < end) {
		struct multiboot_tag* tag = (struct multiboot_tag*)addr;
		switch(tag->type) {
			case MULTIBOOT_TAG_TYPE_END:
				goto done;
			case MULTIBOOT_TAG_TYPE_CMDLINE:
				break;
			case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
				break;
			case MULTIBOOT_TAG_TYPE_MODULE:
				;
				struct multiboot_tag_module* module = (struct multiboot_tag_module*)tag;
				uint64_t* magic = (uint64_t*)module->mod_start;
				uint32_t* magic2 = (uint32_t*)module->mod_start;
				if(*magic == PNKC_MAGIC) {
					kernel_start = module->mod_start;
					kernel_end = module->mod_end;
				} else if(*magic2 == 0x1BADFACE) {
					initrd_start = module->mod_start;
				        initrd_end = module->mod_end;
				}
				break;
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
				break;
			case MULTIBOOT_TAG_TYPE_BOOTDEV:
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
			case MULTIBOOT_TAG_TYPE_VBE:
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
			case MULTIBOOT_TAG_TYPE_APM:
			case MULTIBOOT_TAG_TYPE_EFI32:
			case MULTIBOOT_TAG_TYPE_EFI64:
			case MULTIBOOT_TAG_TYPE_SMBIOS:
			case MULTIBOOT_TAG_TYPE_ACPI_OLD:
			case MULTIBOOT_TAG_TYPE_ACPI_NEW:
			case MULTIBOOT_TAG_TYPE_NETWORK:
			case MULTIBOOT_TAG_TYPE_EFI_MMAP:
			case MULTIBOOT_TAG_TYPE_EFI_BS:
				break;
		}

		addr = (addr + tag->size + 7) & ~7;
	}
done:

	log_32("Check kernel module: 0x", kernel_start);
	print(" ("); print_32(kernel_end - kernel_start); print(")");
	if(kernel_start == 0) {
		log_fail(0x12, "Cannot find kernel");
	} else {
		log_pass();
	}
}

void check_cpuid() {
	uint32_t flags = 0;
	asm volatile("pushfl\n"
		"pop %0\n"
		: "=r"(flags));

	log_32("Check CPUID supported: ", flags);
	if((flags & (1 << 21)) == 0) {
		log_fail(0x20, "CPUID not supported");
	} else {
		log_pass();
	}
}

void check_longmode() {
	uint32_t a = 0x80000000, b, c = 0x00, d;
	cpuid(&a, &b, &c, &d);

	log_32("Check CPUID extention: ", a);
	if(a <= 0x80000000) {
		log_fail(0x30, "CPUID extention not supported");
	} else {
		log_pass();
	}

	a = 0x80000001, c = 0x00;
	cpuid(&a, &b, &c, &d);
	log_32("Check long-mode supported: ", d);

	if((d & (1 << 29)) == 0) {
		log_fail(0x31, "Long-mode not supported");
	} else {
		log_pass();
	}
}

void load_gdt(uint8_t apic_id) {
	extern uint32_t gdtr;
	void lgdt(uint32_t addr);

	if(apic_id == 0) {
		log_32("Load global descriptor table: 0x", (uint32_t)&gdtr);
	}

	lgdt((uint32_t)&gdtr);

	asm volatile("movw %0, %%ax\n"
		"movw %%ax, %%ds\n"
		"movw %%ax, %%es\n"
		"movw %%ax, %%fs\n"
		"movw %%ax, %%gs\n"
		"movw %%ax, %%ss\n"
		: : "r"((uint16_t)0x20));

	void change_cs();
	change_cs();

	if(apic_id == 0) {
		log_pass();
	}
}

/**
 * TLB size: 256KB
 * TLB address: 6MB + apic_id * 2MB - 256KB
 * TLB Blocks(4K blocks)
 * TLB[0] => l2
 * TLB[1] => l3u
 * TLB[2] => l3k
 * TLB[3~61] => l4u
 * TLB[62~63] => l4k
 */
void init_page_tables(uint8_t apic_id) {
	uint32_t base = 0x600000 + apic_id * 0x200000 - 0x40000;
	if(apic_id == 0)
		log_32("Initializing page table: 0x", base);

	PageDirectory* l2 = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L2_INDEX);
	PageDirectory* l3u = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3U_INDEX);
	PageDirectory* l3k = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3K_INDEX);
	PageTable* l4u = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4U_INDEX);
	PageTable* l4k = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4K_INDEX);

	// Clean TLB area
	volatile uint32_t* p = (uint32_t*)base;
	for(int i = 0; i < 65536; i++)
		*p++ = 0;

	// Level 2
	l2[0].base = (uint32_t)l3u >> 12;
	l2[0].p = 1;
	l2[0].us = 1;
	l2[0].rw = 1;

	l2[511].base = (uint32_t)l3k >> 12;
	l2[511].p = 1;
	l2[511].us = 0;
	l2[511].rw = 1;

	// Level 3
	for(int i = 0; i < PAGE_L4U_SIZE; i++) {
		l3u[i].base = (uint32_t)&l4u[i * PAGE_ENTRY_COUNT] >> 12;
		l3u[i].p = 1;
		l3u[i].us = 1;
		l3u[i].rw = 1;
	}

	for(int i = 0; i < PAGE_L4K_SIZE; i++) {
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].base = (uint32_t)&l4k[i * PAGE_ENTRY_COUNT] >> 12;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].p = 1;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].us = 0;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].rw = 1;
	}

	// Level 4
	for(int i = 0; i < PAGE_L4U_SIZE * PAGE_ENTRY_COUNT; i++) {
		l4u[i].base = i;
		l4u[i].p = 1;
		l4u[i].us = 0;
		l4u[i].rw = 1;
		l4u[i].ps = 1;
	}

	// Kernel global area(gmalloc, segment descriptor, IDT, code, rodata)
	// Mapping 256MB to kernel
	for(int i = 0; i < 128; i++) {
		l4k[i].base = i;
		l4k[i].p = 1;
		l4k[i].us = 0;
		l4k[i].rw = 1;
		l4k[i].ps = 1;
		l4k[i].exb = 1;
	}

	// Kernel global area(code, rodata, modules, gmalloc)
	l4k[1].exb = 0;

	// Kernel local area(malloc, TLB, TS, data, bss, stack)
	l4k[2 + apic_id] = l4k[2];

	l4k[2].base = 2 + apic_id;	// 2 * (2 + apic_id)MB
	l4k[2].p = 1;
	l4k[2].us = 0;
	l4k[2].rw = 1;
	l4k[2].ps = 1;

	if(apic_id == 0)
		log_pass();
}

static void clean(void* addr, uint32_t size, uint8_t apic_id) {
	uint32_t* d = (uint32_t*)addr;

	size = (size + 3) / 4;
	uint32_t unit = size / 20;
	unit = unit == 0 ? 1 : unit;
	for(uint32_t i = 0; i < size; i++) {
		/*
		if(*d != 0) {
			print("NOT NULL: ");
			print_32((uint32_t)d);
			print(" ");
			print_32(*d);
		}
		*/
		*d++ = 0;

		if(apic_id == 0 && (i + 1) % unit == 0)
			print(".");
	}
}

static void copy(void* destination, void* source, uint32_t size, uint8_t apic_id) {
	uint32_t* d = (uint32_t*)destination;
	uint32_t* s = (uint32_t*)source;

	size = (size + 3) / 4;
	uint32_t unit = size / 20;
	unit = unit == 0 ? 1 : unit;
	for(uint32_t i = 0; i < size; i++) {
		*d++ = *s++;

		if(apic_id == 0 && (i + 1) % unit == 0)
			print(".");
	}
}

void copy_kernel(uint8_t apic_id) {
	void* pos = (uint32_t*)kernel_start;
	uint32_t size = (uint32_t)(kernel_end - kernel_start);

	PNKC* pnkc = (PNKC*)pos;
	pos += sizeof(PNKC);

	if(apic_id == 0) {
		log_32("Check PacketNgin Kernel Container: ", pnkc->magic >> 32); print_32(pnkc->magic);

		if(size < sizeof(PNKC) && pnkc->magic != PNKC_MAGIC) {
			log_fail(0x100, "Illegal PNKC header");
		} else {
			log_pass();
		}
	}

	if(apic_id == 0) {
		log("Copying kernel:\n");
	}


	// Copy Ramdisk
	if(apic_id == 0) {
		//print("\n    Ramdisk 0x");
// #define RAMDISK_ADDR	(0x400000 + 0x200000 * MP_MAX_CORE_COUNT)
		//print_32(0x2400000); print(" ("); print_32(initrd_end - initrd_start); print(") ");
		copy((void*)0x2400000, (void*)initrd_start, initrd_end - initrd_start, 1);
	}

	// FIXME: caculate size of ramdisk
	uint32_t multiboot_temp_addr = 0x2800000;	// Behind the RAM disk area (+4MB)
	// Clean
	if(apic_id == 0) {
		// Temporarily copy multiboot info
		copy((void*)multiboot_temp_addr, (void*)multiboot2_addr, *(uint32_t*)multiboot2_addr, 1);

		print("    clean 0x00200000 (00400000): ");
		clean((void*)0x200000, 0x200000, apic_id);
	}
	clean((void*)(0x200000 + 0x200000 * (apic_id + 1)), apic_id == 0 ? 0x400000 : 0x200000, apic_id);

	// Copy .text
	if(apic_id == 0) {
		print("\n    .text 0x");
		print_32(0x200000 + pnkc->text_offset); print(" ("); print_32(pnkc->text_size); print(") ");
		copy((void*)0x200000 + pnkc->text_offset, pos, pnkc->text_size, apic_id);
	}
	pos += pnkc->text_size;

	// Copy .rodata
	if(apic_id == 0) {
		print("\n    .rodata: 0x");
		print_32(0x200000 + pnkc->rodata_offset); print(" ("); print_32(pnkc->rodata_size); print(") ");
		copy((void*)0x200000 + pnkc->rodata_offset, pos, pnkc->rodata_size, apic_id);
	}
	pos += pnkc->rodata_size;

	// Copy smap
	if(apic_id == 0) {
		print("\n    smap: 0x");
		print_32(0x200000 + pnkc->smap_offset); print(" ("); print_32(pnkc->smap_size); print(") ");
		copy((void*)0x200000 + pnkc->smap_offset, pos, pnkc->smap_size, apic_id);
	}
	pos += pnkc->smap_size;

	// Copy .data
	if(apic_id == 0) {
		print("\n    .data: 0x");
		print_32(0x400000 + pnkc->data_offset); print(" ("); print_32(pnkc->data_size); print(") ");
	}
	copy((void*)(0x200000 + 0x200000 * (apic_id + 1)) + pnkc->data_offset, pos, pnkc->data_size, apic_id);
	pos += pnkc->data_size;

	// .bss is already inited

	// Write PNKC
	if(apic_id == 0) {
		print("\n    PNKC: 0x");
		print_32(0x200200 - sizeof(PNKC)); print(" ("); print_32(sizeof(PNKC)); print(") ");
		copy((void*)(0x200200 /* Kernel entry end */ - sizeof(PNKC)), pnkc, sizeof(PNKC), apic_id);
	}

	// Write multiboot2 tags
	if(apic_id == 0) {
		uint32_t size = *(uint32_t*)multiboot_temp_addr;
		uint32_t addr = (0x200000 + pnkc->smap_offset + pnkc->smap_size + 7) & ~7;
		//pos = (void*)(((uint32_t)pos + 7) & ~7);

		print("\n    multiboot2: 0x");
		print_32(addr); print(" ("); print_32(size); print(") ");
		copy((void*)addr, (void*)multiboot_temp_addr, size, apic_id);
	}

	if(apic_id == 0) {
		log_pass();
	}
}

void activate_pae(uint8_t apic_id) {
	#define PAE 0x620	// OSXMMEXCPT=1, OSFXSR=1, PAE=1

	uint32_t cr4;
	asm volatile("movl %%cr4, %0" : "=r"(cr4));

	cr4 |= PAE;
	asm volatile("movl %0, %%cr4" : : "r"(cr4));

	if(apic_id == 0) {
		log_32("Activate physical address extension: 0x", cr4);
		log_pass();
	}
}

void activate_pml4(uint8_t apic_id) {
	uint32_t pml4 = 0x5c0000 + 0x200000 * apic_id;
	asm volatile("movl %0, %%cr3" : : "r"(pml4));

	if(apic_id == 0) {
		log_32("Activate PML4 table: 0x", pml4);
		log_pass();
	}
}

void activate_longmode(uint8_t apic_id) {
	#define LONGMODE	0x0901	// NXE=1, LME=1, SCE=1

	uint32_t msr;
	asm volatile("movl %1, %%ecx\n"
		"rdmsr\n"
		"movl %%eax, %0\n"
		: "=r"(msr)
		: "r"(0xc0000080));

	msr |= LONGMODE;

	asm volatile("movl %0, %%eax\n"
		"wrmsr"
		: : "r"(msr));

	if(apic_id == 0) {
		log_32("Activate long-mode: 0x", msr);
		log_pass();
	}
}

void activate_paging(uint8_t apic_id) {
	#define ADD	0xe000003e	// PG=1, CD=1, NW=1, NE=1, TS=1, EM=1, MP=1
	#define REMOVE	0x60000004	//       CD=1, NW=1,             EM=1

	uint32_t cr0;
	asm volatile("movl %%cr0, %0" : "=r"(cr0));
	cr0 |= ADD;
	cr0 ^= REMOVE;

	if(apic_id == 0) {
		log_32("Activate caching and paging: 0x", cr0);
	}

	asm volatile("movl %0, %%cr0" : : "r"(cr0));

	if(apic_id == 0) {
		log_pass();
	}
}


void activate_aps() {
	log("Activate application processors: ");

	print(".");
	apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS |
		APIC_TM_LEVEL |
		APIC_LV_ASSERT |
		APIC_DM_PHYSICAL |
		APIC_DMODE_INIT);

	time_uwait(100);

	if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
		log_fail(0x00, "First INIT IPI is pending");
	}

	print(".");
	apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS |
		APIC_TM_LEVEL |
		APIC_LV_ASSERT |
		APIC_DM_PHYSICAL |
		APIC_DMODE_INIT);

	time_uwait(100);

	if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
		log_fail(0x00, "Second INIT IPI is pending");
	}

	print(".");

	apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS |
		APIC_TM_EDGE |
		APIC_LV_ASSERT |
		APIC_DM_PHYSICAL |
		APIC_DMODE_STARTUP |
		0x10);   // Startup address: 0x10 = 0x10000 / 4KB

	time_uwait(200);

	if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
		log_fail(0x00, "STARTUP IPI is pending");
	}

	log_pass();
}

void main(uint32_t magic, uint32_t addr) {
	uint8_t apic_id = get_apic_id();
	if(apic_id == 0) {
		print_init();

		check_multiboot2(magic, addr);
		check_cpuid();
		check_longmode();
		copy_kernel(apic_id);
		init_page_tables(apic_id);
		load_gdt(apic_id);
		activate_pae(apic_id);
		activate_pml4(apic_id);
		activate_longmode(apic_id);
		activate_paging(apic_id);

		time_init();
		apic_init();
		activate_aps();

		// 64bit kernel arguments
		*(uint32_t*)(0x5c0000 - 0x400) = initrd_start;
		*(uint32_t*)(0x5c0000 - 0x400 + 8) = initrd_end;

		print("Jump to 64bit kernel: 0x00200000");
		tab(75);
	} else {
		copy_kernel(apic_id);
		init_page_tables(apic_id);
		load_gdt(apic_id);
		activate_pae(apic_id);
		activate_pml4(apic_id);
		activate_longmode(apic_id);
		activate_paging(apic_id);
	}
}
