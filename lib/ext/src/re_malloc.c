#include <stdint.h>
#include <string.h>
#include "re_malloc.h"

#if DEBUG || TEST
#include <stdio.h>
#endif /* DEBUG */

#define BLOCK_SIZE	4

typedef struct _Header {
	size_t	count;			// bitmap count
	size_t	index;
	uint8_t	bitmap[0];
} Header;

typedef struct _Block {
	uint32_t	count;		// number of blocks	2^4 * 2^24 = 256MB
} __attribute__ ((packed)) Block;

#define HEADER(pool)		(Header*)((((uintptr_t)(pool) + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE)
#define BASE(header, index)	(void*)((((uintptr_t)(header) + sizeof(Header) + (header)->count + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE + (index) * BLOCK_SIZE)
#define GET(header, index)	((header)->bitmap[(index) / 8] & (1 << ((index) % 8)))
#define SET(header, index)	(header)->bitmap[(index) / 8] |= (1 << ((index) % 8))
#define UNSET(header, index)	(header)->bitmap[(index) / 8] ^= (1 << ((index) % 8))

size_t re_init(void* pool, size_t pool_size) {
	Header* header = HEADER(pool);
	pool_size -= (void*)header - pool;
	
	size_t mempool = ((8 * BLOCK_SIZE * (pool_size - sizeof(Header))) / (8 * BLOCK_SIZE + 1) / BLOCK_SIZE) * BLOCK_SIZE;
	size_t bitmap;
	size_t base;
	size_t end;
	while(1) {
		bitmap = mempool / BLOCK_SIZE / 8;
		base = ((sizeof(Header) + bitmap + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
		end = base + BLOCK_SIZE * bitmap * 8;
		
		if(end <= pool_size)
			break;
		
		mempool -= BLOCK_SIZE;
	}
	
	header->count = bitmap;
	header->index = 0;
	
	bzero(header->bitmap, header->count);
	
	return header->count * 8 * BLOCK_SIZE;
}

static size_t alloc(Header* header, size_t index, size_t count, size_t length) {
	size_t found = 0;
	for(size_t i = 0; i < length; i++) {
		if(!GET(header, index)) {
			found++;
		} else {
			found = 0;
		}
		
		if(++index > header->count * 8) {
			index = 0;
			found = 0;
		}
		
		if(found == 0 || index == 0) {	// Check not found or overflow
			continue;
		}
		
		if(found >= count) {
			return index - found;
		}
	}
	
	return -1;
}

void* re_malloc(size_t size, void* pool) {
	if(size <= 0)
		return NULL;
	
	Header* header = HEADER(pool);
	size_t count = (size + sizeof(Block) + BLOCK_SIZE - 1) / BLOCK_SIZE;
	
	size_t index = alloc(header, header->index, count, header->count * 8);
	if(index == (size_t)-1) {
		return NULL;
	} else {
		for(size_t i = 0; i < count; i++) {
			uint8_t old = header->bitmap[0];
			SET(header, index + i);
		}
		
		Block* block = (Block*)BASE(header, index);
		block->count = count;
		
		header->index = index + count;
		
		return (void*)block + sizeof(Block);
	}
}

void re_free(void *ptr, void* pool) {
	Header* header = HEADER(pool);
	
	if((uintptr_t)ptr < (uintptr_t)BASE(header, 0) || (uintptr_t)ptr >= (uintptr_t)BASE(header, header->count * 8 + 1)) {
		#if DEBUG
		printf("Illegal pointer freeing: %p is not between memory pool: %p ~ %p\n", ptr, BASE(header, 0), BASE(header, header->count * 8 + 1));
		#endif /* DEBUG */
		return;
	}
	
	Block* block = (Block*)(ptr - sizeof(Block));
	#if DEBUG
	if(block->count == 0) {
		printf("Double freeing: %p\n", ptr);
	}
	#endif /* DEBUG */
	 
	size_t count = block->count;
	size_t index = ((uintptr_t)block - ((uintptr_t)pool + sizeof(Header) + header->count)) / BLOCK_SIZE;
	
	for(size_t i = 0; i < count; i++) {
		UNSET(header, index);
		index++;
	}
	
	block->count = 0;
}

void* re_realloc(void *ptr, size_t new_size, void* pool) {
	if(ptr == NULL)
		return re_malloc(new_size, pool);
	
	Header* header = HEADER(pool);
	if((uintptr_t)ptr < (uintptr_t)BASE(header, 0) || (uintptr_t)ptr >= (uintptr_t)BASE(header, header->count * 8 + 1)) {
		#if DEBUG
		printf("Illegal pointer remallocing: %p is not between memory pool: %p ~ %p\n", ptr, BASE(header, 0), BASE(header, header->count * 8 + 1));
		#endif /* DEBUG */
		return NULL;
	}
	
	if(new_size <= 0) {
		re_free(ptr, pool);
		return NULL;
	}
	
	size_t new_count = (new_size + sizeof(Block) + BLOCK_SIZE - 1) / BLOCK_SIZE;
	
	Block* block = (Block*)(ptr - sizeof(Block));
	size_t count = block->count;
	size_t index = ((uintptr_t)block - ((uintptr_t)pool + sizeof(Header) + header->count)) / BLOCK_SIZE;
	
	if(new_count < count) {
		index += count - new_count;
		for(size_t i = 0; i < count - new_count; i++) {
			UNSET(header, index);
			index++;
		}
	} else {
		size_t appendix = alloc(header, index + count, new_count - count, (new_count - count) * 8);
		if(appendix > 0) {
			for(size_t i = 0; i < new_count - count; i++) {
				SET(header, appendix);
				appendix++;
			}
			block->count = new_count;
			
			return ptr;
		} else {
			void* new_ptr = re_malloc(new_size, pool);
			if(new_ptr) {
				memcpy(new_ptr, ptr, count * BLOCK_SIZE);
				re_free(ptr, pool);
				return new_ptr;
			}
		}
	}
	
	return NULL;
}

void* re_calloc(size_t nelem, size_t elem_size, void* pool) {
	if(nelem <= 0 || elem_size <= 0)
		return NULL;
	
	size_t size = nelem * elem_size;
	void* ptr = re_malloc(size, pool);
	if(ptr) {
		bzero(ptr, size);
		return ptr;
	} else {
		return NULL;
	}
}

size_t re_total(void* pool) {
	Header* header = HEADER(pool);
	return header->count * 8 * BLOCK_SIZE;
}

size_t re_used(void* pool) {
	Header* header = HEADER(pool);
	size_t size = 0;
	for(size_t i = 0; i < header->count * 8; i++) {
		if(GET(header, i))
			size++;
	}

	return size * BLOCK_SIZE;
}

size_t re_available(void* pool) {
	Header* header = HEADER(pool);
	size_t size = 0;
	for(size_t i = 0; i < header->count * 8; i++) {
		if(!GET(header, i))
			size++;
	}

	return size * BLOCK_SIZE;
}

#if TEST

#include <stdlib.h>
#include <stdarg.h>

#define BUFFER_SIZE	4096	
uint8_t buffer[BUFFER_SIZE];

static void dump_bitmap(void* pool) {
	Header* header = HEADER(pool);
	for(size_t i = 0; i < header->count * 8; i++) {
		printf("%d", !!GET(header, i));
		if((i + 1) % 64 == 0)
			printf("\n");
	}
	printf("\n");
}

void pass() {
	printf("\tPASSED\n");
}

void fail(const char* format, ...) {
	fflush(stdout);
	fprintf(stderr, "\tFAILED: ");
	
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);

	printf("\n");

	dump_bitmap(buffer);
	
	exit(1);
}

void main() {
	re_init(buffer, BUFFER_SIZE);
	
	printf("buffer: %p ~ %p\n", buffer, &buffer[BUFFER_SIZE]);
	printf("header->count: %ld\n", ((Header*)buffer)->count);
	printf("header->index: %ld\n", ((Header*)buffer)->index);
	printf("header->bitmap: %p ~ %p\n", &((Header*)buffer)->bitmap[0], &((Header*)buffer)->bitmap[((Header*)buffer)->count - 1]);
	printf("header->pool: %p ~ %p\n", BASE((Header*)buffer, 0), BASE((Header*)buffer, ((Header*)buffer)->count * 8 + 1));
	printf("\n");

	printf("Check total size: ");
	size_t total = re_total(buffer);
	printf("%lu", total);
	if(total > (BUFFER_SIZE - BUFFER_SIZE / 10) && total <= BUFFER_SIZE)
		pass();
	else
		fail("Illegal total size\n");
	
	printf("Check available size: ");
	size_t available = re_available(buffer);
	printf("%lu", available);
	if(available == total)
		pass();
	else
		fail("Illegal available size\n");
	
	printf("Check used size: ");
	size_t used = re_used(buffer);
	printf("%lu", used);
	if(used == 0)
		pass();
	else
		fail("Illegal used size\n");
	
	printf("Check first malloc: ");
	void* ptr = re_malloc(4, buffer);
	printf("%p", ptr);
	if(ptr != NULL)
		pass();
	else if(ptr == NULL)
		fail("Allocation failed\n");
	else if((uintptr_t)ptr < (uintptr_t)buffer)
		fail("The pointer underflow the buffer: %p < %p\n", ptr, buffer);
	else if((uintptr_t)ptr > (uintptr_t)buffer + BUFFER_SIZE)
		fail("The pointer overflow the buffer: %p > %p + %d\n", ptr, buffer, BUFFER_SIZE);
	
	printf("Check memory consumption: available size");
	size_t available2 = re_available(buffer);
	if(available2 == available - BLOCK_SIZE - 4)
		pass();
	else
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	
	printf("Check memory consumption: used size");
	size_t used2 = re_used(buffer);
	if(used2 == used + BLOCK_SIZE + 4)
		pass();
	else
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	
	printf("Check second malloc: ");
	void* ptr1 = ptr;
	ptr = re_malloc(8, buffer);
	printf("%p", ptr);
	if(ptr != NULL)
		pass();
	else if(ptr == NULL)
		fail("Allocation failed\n");
	else if((uintptr_t)ptr < (uintptr_t)buffer)
		fail("The pointer underflow the buffer: %p < %p\n", ptr, buffer);
	else if((uintptr_t)ptr > (uintptr_t)buffer + BUFFER_SIZE)
		fail("The pointer overflow the buffer: %p > %p + %d\n", ptr, buffer, BUFFER_SIZE);
	
	printf("Check memory consumption: available size");
	available = available2;
	available2 = re_available(buffer);
	if(available2 == available - BLOCK_SIZE - 8)
		pass();
	else
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	
	printf("Check memory consumption: used size");
	used = used2;
	used2 = re_used(buffer);
	if(used2 == used + BLOCK_SIZE + 8)
		pass();
	else
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	
	printf("Check first free");
	re_free(ptr1, buffer);

	available = available2;
	available2 = re_available(buffer);
	if(available2 != available + BLOCK_SIZE + 4)
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	
	used = used2;
	used2 = re_used(buffer);
	if(used2 != used - BLOCK_SIZE - 4)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	printf("Check second free");
	re_free(ptr, buffer);

	 
	available = available2;
	available2 = re_available(buffer);
	if(available2 != available + BLOCK_SIZE + 8)
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	
	used = used2;
	used2 = re_used(buffer);
	if(used2 != used - BLOCK_SIZE - 8)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	printf("Aligned allocation: 4 bytes");
	re_init(buffer, BUFFER_SIZE);
	void* ptrs[BUFFER_SIZE / 2] = { 0, };
	size_t size = re_total(buffer) / (BLOCK_SIZE + 4);
	for(int i = 0; i < size; i++) {
		ptrs[i] = re_malloc(4, buffer);
		if(!ptrs[i]) {
			fail("Memory not allocated: [%d] = NULL\n", i);
		}
	}
	
	pass();
	 
	printf("Aligned allocation: available size ");
	available = available2;
	available2 = re_available(buffer);
	printf("%ld", available2);
	if(available2 != available - (BLOCK_SIZE + 4) * size)
		fail("Expected available size: %lu\n", available - (BLOCK_SIZE + 4) * size);
	else
		pass();
	
	printf("Aligned allocation: used size ");
	used = used2;
	used2 = re_used(buffer);
	printf("%ld", used2);
	if(used2 != used + (BLOCK_SIZE + 4) * size)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	else
		pass();
	
	printf("Aligned overflow malloc: 4 bytes");
	for(int i = 0; i < 32; i++) {
		ptr = re_malloc(4, buffer);
		if(ptr) {
			fail("Aligned overflow malloced: [%d] = %p\n", i, ptr);
		}
	}

	pass();
	
	printf("Aligned overflow: available size ");
	size_t available3 = re_available(buffer);
	printf("%ld", available3);
	if(available3 != available2)
		fail("New available size: %lu, old available size: %lu\n", available3, available2);
	else
		pass();
	
	printf("Aligned overflow: used size ");
	size_t used3 = re_used(buffer);
	printf("%ld", used3);
	if(used3 != used2)
		fail("New used size: %lu, old used size: %lu\n", used3, used2);
	else
		pass();
	 
	printf("Aligned free: 4 bytes");
	for(int i = 0; i < size; i++) {
		re_free(ptrs[i], buffer);
	}

	pass();
	
	printf("Aligned allocation: available size ");
	available2 = re_available(buffer);
	printf("%ld", available2);
	if(available2 != available)
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	else
		pass();
	
	printf("Aligned allocation: used size ");
	used2 = re_used(buffer);
	printf("%ld", used2);
	if(used2 != used)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	else
		pass();
	
	printf("Unaligned allocation: 7 bytes");
	re_init(buffer, BUFFER_SIZE);
	size = re_total(buffer) / (BLOCK_SIZE + ((7 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE);
	for(int i = 0; i < size; i++) {
		ptrs[i] = re_malloc(7, buffer);
		if(!ptrs[i]) {
			fail("Memory not allocated: [%d] = NULL\n", i);
		}
	}
	
	pass();
	 
	printf("Unaligned allocation: available size ");
	available = available2;
	available2 = re_available(buffer);
	printf("%ld", available2);
	if(available2 != available - (BLOCK_SIZE + 8) * size)
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	else
		pass();
	
	printf("Unaligned allocation: used size ");
	used = used2;
	used2 = re_used(buffer);
	printf("%ld", used2);
	if(used2 != used + (BLOCK_SIZE + 8) * size)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	else
		pass();
	
	printf("Unaligned overflow malloc: 7 bytes");
	for(int i = 0; i < 32; i++) {
		ptr = re_malloc(7, buffer);
		if(ptr) {
			fail("Unaligned overflow malloced: [%d] = %p\n", i, ptr);
		}
	}

	pass();
	
	printf("Unaligned overflow: available size ");
	available3 = re_available(buffer);
	printf("%ld", available3);
	if(available3 != available2)
		fail("New available size: %lu, old available size: %lu\n", available3, available2);
	else
		pass();
	
	printf("Unaligned overflow: used size ");
	used3 = re_used(buffer);
	printf("%ld", used3);
	if(used3 != used2)
		fail("New used size: %lu, old used size: %lu\n", used3, used2);
	else
		pass();
	 
	printf("Unaligned free: 7 bytes");
	for(int i = 0; i < size; i++) {
		re_free(ptrs[i], buffer);
	}

	pass();
	
	printf("Unaligned allocation: available size ");
	available2 = re_available(buffer);
	printf("%ld", available2);
	if(available2 != available)
		fail("New available size: %lu, old available size: %lu\n", available2, available);
	else
		pass();
	
	printf("Unaligned allocation: used size ");
	used2 = re_used(buffer);
	printf("%ld", used2);
	if(used2 != used)
		fail("New used size: %lu, old used size: %lu\n", used2, used);
	else
		pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear aligned allocations and freeing: 4 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(4, buffer);
		}
		for(int i = 0; i < 10; i++) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear aligned allocations and backward freeing: 4 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE * 100; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(4, buffer);
		}
		for(int i = 9; i >= 0; i--) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear unaligned allocations and freeing: 7 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE * 100; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 0; i < 10; i++) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear unaligned allocations and backward freeing: 7 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE * 100; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 9; i >= 0; i--) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear half aligned allocations and freeing: 4 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE * 100; i++) {
		for(int i = 0; i < 20; i++) {
			ptrs[i] = re_malloc(4, buffer);
		}
		for(int i = 0; i < 10; i++) {
			re_free(ptrs[i], buffer);
		}
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(4, buffer);
		}
		for(int i = 10; i < 20; i++) {
			re_free(ptrs[i], buffer);
		}
		for(int i = 10; i < 20; i++) {
			ptrs[i] = re_malloc(4, buffer);
		}
		for(int i = 0; i < 20; i++) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear half aligned allocations and backward freeing: 4 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE; i++) {
		for(int i = 0; i < 20; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 0; i < 10; i++) {
			re_free(ptrs[i], buffer);
		}
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 10; i < 20; i++) {
			re_free(ptrs[i], buffer);
		}
		for(int i = 10; i < 20; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 0; i < 20; i++) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear half unaligned allocations and freeing: 7 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 0; i < 10; i++) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
	
	re_init(buffer, BUFFER_SIZE);
	printf("Linear half unaligned allocations and backward freeing: 7 ");
	available = re_available(buffer);
	used = re_used(buffer);
	for(int i = 0; i < BUFFER_SIZE; i++) {
		for(int i = 0; i < 10; i++) {
			ptrs[i] = re_malloc(7, buffer);
		}
		for(int i = 9; i >= 0; i--) {
			re_free(ptrs[i], buffer);
		}
	}
	available2 = re_available(buffer);
	used2 = re_used(buffer);
	
	if(available != available2)
		fail("Avaiable memory size is differ: new available size: %lu, old available size: %lu\n", available2, available);

	if(used != used2)
		fail("Used memory size is differ: new used size: %lu, old used size: %lu\n", used2, used);
	
	pass();
}
#endif /* TEST */
