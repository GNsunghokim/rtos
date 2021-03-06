#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <util/map.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <vnic.h>

#include <control/vmspec.h>
#include <startup.h>

/* Don't Fix this structure */
typedef struct _PNDShareData {
	uint64_t    PHYSICAL_OFFSET;
	Map*        vms;
	ssize_t     pool_size;
	uint8_t     padding[0] __attribute__((__aligned__(4096)));
	uint8_t     pool[0];
} __attribute__ ((packed)) PNDShareData;

/* Don't Fix this structure */
#define MP_MAX_CORE_COUNT	16
typedef struct {
	uint32_t	count;
	void**		blocks;				// gmalloc(array), bmalloc(content)
} Block;

typedef struct _VM {
	uint32_t	id;					///< VM identifier
	int		    core_size;			///< Number of cores
	uint8_t		cores[MP_MAX_CORE_COUNT];	///< Set of core id
	Block		memory;				///< Total Memeory size
	Block		storage;			///< Total Block size
	uint64_t	used_size;			///< Application image size
	int	    	nic_count;			///< Number of NICs
	VNIC**		nics;				///< NICs (gmalloc)
	//	VFIO*		fio;
	int	    	argc;				///< Number of arguments
	char**		argv;				///< Arguments (gmalloc)

	VMStatus	status;				///< VM status
} VM;

static bool pnd_is_loaded;
static PNDShareData* pnd_share_data;

#define PNDSHM_NAME              	"/pndshm"
#define PNDSHM_SIZE 	            0x1000000 //16Mbyte
#define PNDSHM_OFFSET				0xff000000

#define MANAGER_PHYSICAL_OFFSET		0xff00000000
#define MAPPING_AREA_SIZE	    	0x80000000	/* 2 GB */

static bool pn_load() {
	int mem_fd = 0;
	int shm_fd = shm_open(PNDSHM_NAME, O_RDONLY, 0666);
	if(shm_fd < 0) goto error;

	pnd_share_data = mmap((void*)PNDSHM_OFFSET, PNDSHM_SIZE,
			PROT_READ, MAP_SHARED | MAP_FIXED, shm_fd, 0);

	if(pnd_share_data == MAP_FAILED) goto error;

	mem_fd = open("/dev/mem", O_RDONLY | O_SYNC);
	if(mem_fd < 0) goto error;

	void* mapping = mmap((void*)MANAGER_PHYSICAL_OFFSET + 0x100000,
			MAPPING_AREA_SIZE, PROT_READ,
			MAP_SHARED | MAP_FIXED, mem_fd, (off_t)pnd_share_data->PHYSICAL_OFFSET + 0x100000);

	if(mapping == MAP_FAILED) goto error;

	if(mapping != (void*)(MANAGER_PHYSICAL_OFFSET + 0x100000)) goto error;

	pnd_is_loaded = true;

	return true;

error:
	if(mem_fd > 0) close(mem_fd);

	if(shm_fd > 0) close(shm_fd);

	return false;
}

static void pn_unload() {
	if(pnd_is_loaded) {
		munmap((void*)MANAGER_PHYSICAL_OFFSET + 0x100000, MAPPING_AREA_SIZE);
		munmap((void*)PNDSHM_OFFSET, PNDSHM_SIZE);
		pnd_is_loaded = false;
	}
}

static void** get_memory_blocks(VM* vm) {
	return (void**)((uint64_t)MANAGER_PHYSICAL_OFFSET | (uint64_t)vm->memory.blocks);
}

static uint64_t _map_uint64_hash(void* key) {
	return (uintptr_t)key;
}

static bool _map_uint64_equals(void* key1, void* key2) {
	return key1 == key2;
}

static void* _map_get(Map* map, void* key) {
	size_t index = _map_uint64_hash(key) % map->capacity;
	if(!map->table[index]) {
		return NULL;
	}

	size_t size = list_size(map->table[index]);
	for(size_t i = 0; i < size; i++) {
		MapEntry* entry = list_get(map->table[index], i);
		if(_map_uint64_equals(entry->key, key))
			return entry->data;
	}

	return NULL;
}

static VM* pnd_share_data_get_vm(int vmid) {
	if(!pnd_share_data) return NULL;

	VM* vm = _map_get(pnd_share_data->vms, (void*)(uint64_t)vmid);
	if(!vm) return NULL;

	vm = (VM*)(MANAGER_PHYSICAL_OFFSET | (uint64_t)vm);

	return vm;
}

typedef struct {
	uint8_t		barrior_lock;
	uint32_t	barrior;
	uint8_t		shared[64 * 1024];
} SharedBlock;

void vm_info(int vmid) {
	VM* vm = pnd_share_data_get_vm(vmid);
	if(!vm) return;

	printf("\n**** RTVM Information ****\n");
	printf("VMID:\t\t%d\n", vm->id);
	printf("Core Size: %d\n", vm->core_size);
	printf("Cores:\t\t");
	for(int i = 0; i < vm->core_size; i++)
		printf("[%d]", vm->cores[i]);
	printf("\n");
}

int vm_status(int vmid) {
	VM* vm = pnd_share_data_get_vm(vmid);
	if(!vm) return VM_STATUS_INVALID;

	return vm->status;
}

#define MAX_MEMORY_BLOCK	256
static void* block_list[MAX_MEMORY_BLOCK];
static int block_list_count = 0;

static void unmapping_global_heap() {
	while(block_list_count) {
		void* mapping = block_list[--block_list_count];
		printf("Virtual memory unmap: %dMB\n", mapping);
		munmap(mapping, 0x200000);
	}
}

static bool mapping_global_heap(int vmid) {	//mapping memory pool
	VM* vm = pnd_share_data_get_vm(vmid);
	if(!vm) return false;

	void** memory = get_memory_blocks(vm);
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) return false;

	for(uint32_t j = 0, i = 1 + 1 + vm->core_size; i < vm->memory.count; i++, j++) {
		if(block_list_count == MAX_MEMORY_BLOCK) goto fail;

		// TODO fix here
		void* mapping = mmap((void*)0xe00000 + 0x200000 * j,
				0x200000, PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_FIXED, fd, (off_t)(memory[i] + pnd_share_data->PHYSICAL_OFFSET));

		printf("Virtual memory map: %dMB -> %dMB Global Heap\n", (0xe00000 + 0x200000 * j) / 0x100000, ((off_t)(memory[i] + pnd_share_data->PHYSICAL_OFFSET)) / 0x100000);

		if(mapping == MAP_FAILED) goto fail;

		if(mapping != (void*)0xe00000 + 0x200000 * j) goto fail;

		block_list[block_list_count++] = mapping;
	}

	SharedBlock* shared_block = (void*)0xe00000;
	extern void* __gmalloc_pool;
	extern void* __shared;
	__shared = shared_block->shared;
	__gmalloc_pool = (void*)0xe00000 + sizeof(SharedBlock);

	close(fd);
	return true;

fail:
	unmapping_global_heap();
	close(fd);
	return false;
}

#define DEFAULT_VMID	1

void _startup(int vmid) {
	if(!pn_load()) exit(-1);

	if(!mapping_global_heap(vmid)) exit(-1);
}

void startup(int argc, char** argv) {
	static struct option options[] = {
		{ "vmid", required_argument, 0, 'v' },
		{ 0, 0, 0, 0 }
	};

	uint32_t vmid = DEFAULT_VMID;

	int opt;
	int index = 0;
	while((opt = getopt_long(argc, argv, "v:", options, &index)) != -1) {
		switch(opt) {
			case 'v':
				vmid = strtoul(optarg, NULL, 10);
				break;
		}
	}

	_startup(vmid);
}

void shutdown() {
	unmapping_global_heap();
	pn_unload();
}
