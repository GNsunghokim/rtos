#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <malloc.h>
#include <_malloc.h>
#include <lock.h>
#include <string.h>
#include <timer.h>
#include <net/crc.h>
#include <util/fifo.h>
#include <net/arp.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/ip.h>
#define POOL_SIZE	0x40000

extern int __nic_count;
extern NIC* __nics[NIC_SIZE];

typedef struct {
	uint64_t	mac;
	uint64_t	timeout;
} ARPEntity;

/**
 * Sample ARP request packet
 * Ethernet (len 6), IPv4 (len 4), Request who-has 192.168.10.144 tell 192.168.10.111, length 46
 */
uint8_t arp_request_packet[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc3, 
		0x7b, 0x93, 0x09, 0xd1, 0x08, 0x06, 0x00, 0x01,
		0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x10, 0xc3,
		0x7b, 0x93, 0x09, 0xd1, 0xc0, 0xa8, 0x0a, 0x6f,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8,
		0x0a, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00
};

/**
 * Sample ARP reply packet
 * Ethernet (len 6), IPv4 (len 4), Reply 192.168.10.144 is-at 74:d4:35:8f:66:cb, length 28
 */
uint8_t arp_reply_packet[] = {
		0x10, 0xc3, 0x7b, 0x93, 0x09, 0xd1, 0x74, 0xd4, 
		0x35, 0x8f, 0x66, 0xcb, 0x08, 0x06, 0x00, 0x01,
		0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x74, 0xd4, 
		0x35, 0x8f, 0x66, 0xcb, 0xc0, 0xa8, 0x0a, 0x90,
		0x10, 0xc3, 0x7b, 0x93, 0x09, 0xd1, 0xc0, 0xa8,
		0x0a, 0x6f
};

/**
 * Sample ARP reply packet for arp announcement
 * Ethernet (len 6), IPv4 (len 4), Reply 192.168.10.111 is-at 10:c3:7b:93:09:d1, length 28
 * NOTE: The target mac address of this packet is modified.
 */
uint8_t arp_announce_packet[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc3,  
		0x7b, 0x93, 0x09, 0xd1, 0x08, 0x06, 0x00, 0x01,
		0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x10, 0xc3,
		0x7b, 0x93, 0x09, 0xd1, 0xc0, 0xa8, 0x0a, 0x6f,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 
		0x0a, 0x6f 
};



static void arp_process_func(void** state) {
	// Timer setting
	timer_init("IntelCore(TM) i5-4670 CPU @ 3.40GHz");

	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x74d4358f66cb;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);


	// Arp request
	Packet* packet = nic_alloc(__nics[0], sizeof(arp_request_packet));
	memcpy(packet->buffer + packet->start, arp_request_packet, sizeof(arp_request_packet));

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ARP* arp = (ARP*)ether->payload;
	uint32_t addr = endian32(arp->tpa);
	nic_ip_add(__nics[0], addr);

	assert_true(arp_process(packet));
	packet = fifo_pop(__nics[0]->output_buffer); 
	assert_memory_equal(packet->buffer + packet->start, arp_reply_packet, 42);

	nic_free(packet);
	packet = NULL;
	
	// Arp response
	packet = nic_alloc(__nics[0], sizeof(arp_reply_packet));
	memcpy(packet->buffer + packet->start, arp_reply_packet, sizeof(arp_reply_packet));

	ether = (Ether*)(packet->buffer + packet->start);
	arp = (ARP*)ether->payload;
	addr = endian32(arp->tpa);
	nic_ip_add(__nics[0], addr);

	uint8_t comp_mac[6] = { 0xcb, 0x66, 0x8f, 0x35, 0xd4, 0x74 }; 
	uint32_t sip = endian32(arp->spa);
	Map* arp_table = nic_config_get(packet->nic, "net.arp.arptable");
	assert_true(arp_process(packet));
	ARPEntity* entity = map_get(arp_table, (void*)(uintptr_t)sip);

	assert_memory_equal((uint8_t*)&entity->mac, comp_mac, 6);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void arp_request_func(void** state) {
	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x10c37b9309d1;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	uint32_t spa = 0xc0a80a6f;
	uint32_t dpa = 0xc0a80a90;
	arp_request(__nics[0], dpa, spa);

	Packet* packet = fifo_pop(__nics[0]->output_buffer); 

	assert_memory_equal(packet->buffer + packet->start, arp_request_packet, 42);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void arp_announce_func(void** state) {
	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x10c37b9309d1;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	uint32_t spa = 0xc0a80a6f;
	arp_announce(__nics[0], spa);

	Packet* packet = fifo_pop(__nics[0]->output_buffer); 
	
	assert_memory_equal(packet->buffer + packet->start, arp_announce_packet, 46);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void arp_get_mac_func(void** state) {
	//timer setting
	timer_init("IntelCore(TM) i5-4670 CPU @ 3.40GHz");

	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x74d4358f66cb;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	Packet* packet = nic_alloc(__nics[0], sizeof(arp_reply_packet));
	memcpy(packet->buffer + packet->start, arp_reply_packet, sizeof(arp_reply_packet));

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ARP* arp = (ARP*)ether->payload;
	uint32_t addr = endian32(arp->tpa);

	nic_ip_add(__nics[0], addr);

	if(!arp_process(packet))
		return;

	uint32_t spa = 0xc0a80a6f;
	uint32_t dpa = 0xc0a80a90;

	uint64_t comp_tha = 0x74d4358f66cb;
	uint64_t d_mac = arp_get_mac(__nics[0], dpa, spa);

	assert_memory_equal((uint8_t*)&d_mac, (uint8_t*)&comp_tha, 6);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void arp_get_ip_func(void** state) {
	//timer setting
	timer_init("IntelCore(TM) i5-4670 CPU @ 3.40GHz");

	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x74d4358f66cb;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	Packet* packet = nic_alloc(__nics[0], 42);
	memcpy(packet->buffer + packet->start, arp_reply_packet, 42);

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ARP* arp = (ARP*)ether->payload;
	uint32_t addr = endian32(arp->tpa);

	nic_ip_add(__nics[0], addr);

	if(!arp_process(packet))
		return;

	uint32_t dpa = 0xc0a80a90;
	uint64_t tha = 0x74d4358f66cb;
	uint32_t comp_ip = arp_get_ip(__nics[0], tha);

	
	assert_memory_equal((uint8_t*)&dpa, (uint8_t*)&comp_ip, 4);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

static void arp_pack_func(void** state) {
	// Nic initialization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->mac = 0x10c37b9309d1;
	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(8, malloc_pool);
	__nics[0]->config = map_create(8, NULL, NULL, __nics[0]->pool);

	uint32_t spa = 0xc0a80a6f;
	uint32_t dpa = 0xc0a80a90;
	arp_request(__nics[0], dpa, spa);

	Packet* packet = fifo_pop(__nics[0]->output_buffer); 
	uint32_t comp_size = packet->end;
	packet->end = 0;

	arp_pack(packet);

	// Checking packet->end
	assert_int_equal(comp_size, packet->end);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(arp_process_func),
		cmocka_unit_test(arp_request_func),
		cmocka_unit_test(arp_announce_func),
		cmocka_unit_test(arp_get_mac_func),
		cmocka_unit_test(arp_get_ip_func),
		cmocka_unit_test(arp_pack_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
