// Standard C
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Core
#include <util/cmd.h>
#include <util/event.h>

// Kernel
#include "asm.h"
#include "timer.h"
#include "malloc.h"
#include "gmalloc.h"
#include "page.h"
#include "stdio.h"
#include "port.h"
#include "mp.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "acpi.h"
#include "pci.h"
#include "apic.h"
#include "ioapic.h"
#include "task.h"
#include "icc.h"
#include "symbols.h"
#include "file.h"
#include "module.h"
#include "device.h"
#include "vnic.h"
#include "manager.h"
#include "shell.h"
#include "loader.h"
#include "vfio.h"
#include "shared.h"
#include "pnkc.h"
#include "mmap.h"
#include "version.h"
#include "rtc.h"

// Drivers
#include "driver/nicdev.h"
#include "driver/pata.h"
#include "driver/usb/usb.h"
#include "driver/ramdisk.h"
#include "driver/virtio_blk.h"
#include "driver/fs.h"
#include "driver/bfs.h"
#include "driver/console.h"

static void ap_timer_init() {
	extern uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t __timer_ms;
	extern uint64_t __timer_us;
	extern uint64_t __timer_ns;

	*(uint64_t*)&TIMER_FREQUENCY_PER_SEC = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&TIMER_FREQUENCY_PER_SEC);
	__timer_ms = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_ms);
	__timer_us = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_us);
	__timer_ns = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_ns);
}

static bool idle0_event(void* data) {
	// #ifdef VFIO_ENABLED
	// 	// Poll FIO
	// #define MAX_VM_COUNT	128
	// 	uint32_t vmids[MAX_VM_COUNT];
	// 	int vm_count = vm_list(vmids, MAX_VM_COUNT);
	// 	for(int i = 0; i < vm_count; i++) {
	// 		VM* vm = vm_get(vmids[i]);
	// 		VFIO* fio = vm->fio;
	// 		if(!fio)
	// 			continue;
	// 
	// 		if(!fio->input_addr)
	// 			continue;
	// 
	// 		// Check if user changed request_id on purpose, and fix it
	// 		if(fio->user_fio->request_id != fio->request_id + fifo_size(fio->input_addr))
	// 			fio->user_fio->request_id = fio->request_id + fifo_size(fio->input_addr);
	// 
	// 		// Check if there's something in the input fifo
	// 		if(fifo_size(fio->input_addr) > 0)
	// 			vfio_poll(vm);
	// 	}
	// #endif
	// 
	// 	// idle
	// 	for(int i = 0; i < 1000; i++)
	// 		asm volatile("nop");

	// idle_time += cpu_tsc() - time;
	return true;
}

static bool idle_monitor_event(void* data) {
	static uint8_t trigger;

	monitor(&trigger);
	mwait(1, 0x21);

	return true;
}

static bool idle_hlt_event(void* data) {
	hlt();

	return true;
}

static int print_version(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("%d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);

	return 0;
}

static Command commands[] = {
	{
		.name = "version",
		.desc = "Print the kernel version.",
		.func = print_version
	},
};

static void context_switch() {
	// Set exception handlers
	APIC_Handler old_exception_handlers[32];

	void exception_handler(uint64_t vector, uint64_t err) {
		if(apic_user_rip() == 0 && apic_user_rsp() == task_get_stack(1)) {
			// Do nothing
		} else {
			printf("* User VM exception handler");
			apic_dump(vector, err);
			errno = err;
		}

		apic_eoi();

		task_destroy(1);
	}

	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			old_exception_handlers[i] = apic_register(i, exception_handler);
		}
	}

	// Context switching
	// TODO: Move exception handlers to task resources
	task_switch(1);

	// Restore exception handlers
	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			apic_register(i, old_exception_handlers[i]);
		}
	}

	// Send callback message
	bool is_paused = errno == 0 && task_is_active(1);
	if(is_paused) {
		// ICC_TYPE_PAUSE is not a ICC message but a interrupt in fact, 
		// so forcely commit the message
	}

	ICC_Message* msg3 = icc_alloc(is_paused ? ICC_TYPE_PAUSED : ICC_TYPE_STOPPED);
	msg3->result = errno;
	if(!is_paused) {
		msg3->data.stopped.return_code = apic_user_return_code();
	}
	errno = 0;
	icc_send(msg3, 0);

	printf("VM %s...\n", is_paused ? "paused" : "stopped");
}

static void icc_start(ICC_Message* msg) {
	VM* vm = msg->data.start.vm;
	printf("Loading VM... \n");

	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);

	if(errno != 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);

		msg2->result = errno;	// errno from loader_load
		icc_send(msg2, msg->apic_id);
		icc_free(msg);
		printf("Execution FAILED: %x\n", errno);
		return;
	}

	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_count;
	NIC** nics = (NIC**)task_addr(id, SYM_NIS);
	for(int i = 0; i < vm->nic_count; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nics[i] = vm->nics[i]->nic;
	}

	printf("Starting VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);

	msg2->data.started.stdin = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDIN));
	msg2->data.started.stdin_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_HEAD));
	msg2->data.started.stdin_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_TAIL));
	msg2->data.started.stdin_size = *(int*)task_addr(id, SYM_STDIN_SIZE);

	msg2->data.started.stdout = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT));
	msg2->data.started.stdout_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD));
	msg2->data.started.stdout_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL));
	msg2->data.started.stdout_size = *(int*)task_addr(id, SYM_STDOUT_SIZE);
	msg2->data.started.stderr = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDERR));
	msg2->data.started.stderr_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_HEAD));
	msg2->data.started.stderr_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_TAIL));
	msg2->data.started.stderr_size = *(int*)task_addr(id, SYM_STDERR_SIZE);

	msg2->data.started.global_heap_idx = TRANSLATE_TO_PHYSICAL((uint64_t)*(uint64_t*)task_addr(id, SYM_GMALLOC_POOL)) >> 21;

	icc_send(msg2, msg->apic_id);

	icc_free(msg);

	context_switch();
}

static void icc_resume(ICC_Message* msg) {
	if(msg->result < 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	printf("Resuming VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);

	icc_send(msg2, msg->apic_id);
	icc_free(msg);
	context_switch();
}

static void icc_pause(uint64_t vector, uint64_t error_code) {
	apic_eoi();
	task_switch(0);
}

static void icc_stop(ICC_Message* msg) {
	if(msg->result < 0) { // Not yet core is started.
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOPPED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	icc_free(msg);
	task_destroy(1);
}

static void fixup_page_table(uint8_t apic_id, uint64_t offset) {
	uint64_t base = VIRTUAL_TO_PHYSICAL(PAGE_TABLE_START) + apic_id * 0x200000 + offset;
	PageTable* l4u = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4U_INDEX);

	for(int i = 0; i < PAGE_L4U_SIZE * PAGE_ENTRY_COUNT; i++) {
		if(i == 0)
			continue;

		l4u[i].base = i + (offset >> 21);
		l4u[i].p = 1;
		l4u[i].us = 0;
		l4u[i].rw = 1;
		l4u[i].ps = 1;
	}

	// Local APIC address (0xfee00000: 0x7F7(PFN))
	l4u[0x7f7].base = 0x7f7;

	uint64_t pml4 = VIRTUAL_TO_PHYSICAL(PAGE_TABLE_START) + apic_id * 0x200000 + offset;
	asm volatile("movq %0, %%cr3" : : "r"(pml4));
}

void main() {
	shared_init();
	amp_init(NULL);
	mp_sync();	// Barrier #0
	mp_init();

	uint64_t apic_id = mp_apic_id();

	extern uint64_t PHYSICAL_OFFSET;
	fixup_page_table(apic_id, PHYSICAL_OFFSET);
	console_init();

	uint64_t vga_buffer = (uint64_t)VGA_BUFFER_START;

	stdio_init(apic_id, (void*)vga_buffer,  VGA_BUFFER_END - VGA_BUFFER_START);

	malloc_init();

	rtc_init();

	mp_sync();	// Barrier #1
	if(apic_id == 0) {
		printf("\nPacketNgin ver 2.0.\n");
		printf("\x1b""32mOK""\x1b""0m\n");

		PNKC* pnkc = (PNKC*)(0x200000 - sizeof(PNKC));

		printf("Analyze CPU information...\n");
		cpu_init();
		gmalloc_init();
		timer_init();
		vnic__init_timer(TIMER_FREQUENCY_PER_SEC);

		gdt_init();
		tss_init();
		idt_init();

		mp_sync();	// Barrier #2

		printf("Loading GDT...\n");
		gdt_load();

		printf("Loading TSS...\n");
		tss_load();

		printf("Loading IDT...\n");
		idt_load();

		printf("Initializing APICs...\n");
		apic_activate();

		printf("Initializing PCI...\n");
		pci_init();

		printf("Initailizing local APIC...\n");
		apic_init();

		printf("Initializing I/O APIC...\n");
		ioapic_init();
		apic_enable();

		printf("Initializing Multi-tasking...\n");
		task_init();

		printf("Initializing events...\n");
		if(!event_init()) {
			printf("Event init problem\n");
			while(1);
		}

		printf("Initializing inter-core communications...\n");
		icc_init();

		printf("Initializing USB controller driver...\n");
		usb_initialize();

		printf("Initializing disk drivers...\n");
		disk_init();
		if(!disk_register(&pata_driver, NULL)) {
			printf("\tPATA driver registration FAILED!\n");
			while(1) asm("hlt");
		}

		if(!disk_register(&usb_msc_driver, NULL)) {
			printf("\tUSB MSC driver registration FAILED!\n");
			while(1) asm("hlt");
		}

		if(!disk_register(&virtio_blk_driver, NULL)) {
			printf("\tVIRTIO BLOCK driver registration FAILED!\n");
			while(1) asm("hlt");
		}

		printf("Initializing RAM disk...\n");
		char cmdline[64];
		sprintf(cmdline, "-addr 0x%lx -size 0x%lx", RAMDISK_START, pnkc->initrd_end - pnkc->initrd_start);
		if(!disk_register(&ramdisk_driver, cmdline)) {
			printf("\tRAM disk driver registration FAILED!\n");
			while(1) asm("hlt");
		}

		printf("Initializing file system...\n");
		fs_init();
		fs_register(&bfs_driver);
		fs_mount(DISK_TYPE_RAMDISK << 16 | 0x00, 0,  FS_TYPE_BFS, "/boot");

		printf("Initializing kernel symbols...\n");
		symbols_init();

		printf("Initializing modules...\n");
		module_init();

		printf("Initializing device drivers...\n");
		device_module_init();

		printf("Initializing VM manager...\n");
		vm_init();

		printf("Initializing RPC manager...\n");
		manager_init();

		printf("Initializing shell...\n");
		shell_init();

		//event_busy_add(idle0_event, NULL);
	} else {
		mp_sync();	// Barrier #2
		ap_timer_init();

		gdt_load();
		tss_load();
		idt_load();

		apic_init();
		apic_enable();

		task_init();
		event_init();
		icc_init();
		icc_register(ICC_TYPE_START, icc_start);
		icc_register(ICC_TYPE_RESUME, icc_resume);
		icc_register(ICC_TYPE_STOP, icc_stop);
		apic_register(49, icc_pause);

		if(cpu_has_feature(CPU_FEATURE_MONITOR_MWAIT) && cpu_has_feature(CPU_FEATURE_MWAIT_INTERRUPT))
			event_idle_add(idle_monitor_event, NULL);
		else
			event_idle_add(idle_hlt_event, NULL);
	}

	mp_sync(); // Barrier #3

	// 	if(apic_id == 0) {
	// 	        while(exec("/boot/init.psh") > 0)
	// 	                event_loop();
	// 	}

	cmd_register(commands, sizeof(commands) / sizeof(commands[0]));

	while(1) {
		event_loop();
	}
}
