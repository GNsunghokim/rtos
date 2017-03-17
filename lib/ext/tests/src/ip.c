#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <malloc.h>
#include <_malloc.h>
#include <lock.h>

#include <net/icmp.h>
#include <util/fifo.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/ip.h>

#define POOL_SIZE	0x40000

extern int __nic_count;
extern NIC* __nics[NIC_SIZE];

// sample tcp packet.(66 byte)
uint8_t sample_tcp_packet[] = {	
			0x90, 0x9f, 0x33, 0xa3, 0xc5, 0x70, 0x10, 0xc3, 
			0x7b, 0x93, 0x09, 0xd1, 0x08, 0x00, 0x45, 0x00, 
			0x00, 0x34,	0x0d, 0x76, 0x40, 0x00, 0x40, 0x06,
			0x9e, 0xd5, 0xc0, 0xa8, 0x0a, 0x6f, 0x60, 0x10,
			0x63, 0x51, 0xba, 0xba, 0x00, 0x50, 0xc3, 0x69,
			0x81, 0xf8, 0x56, 0x8a, 0x7e, 0xa6, 0x80, 0x10,
			0x00, 0xe5, 0x65, 0xd8, 0x00, 0x00, 0x01, 0x01,
			0x08, 0x0a, 0x00, 0xd0, 0x8d, 0x8b, 0xac, 0x97,
			0x70, 0xf6
};

// sample icmp packet.(98 byte)
uint8_t sample_icmp_packet[] = {
			0x10, 0xc3, 0x7b, 0x93, 0x09, 0xd1, 0x74, 0xd4,
			0x35, 0x8f, 0x66, 0xbb, 0x08, 0x00, 0x45, 0x00, 
			0x00, 0x54, 0x48, 0xe9, 0x00, 0x00, 0x40, 0x01,
			0x9b, 0x9b, 0xc0, 0xa8, 0x0a, 0x65, 0xc0, 0xa8,
			0x0a, 0x6f, 0x00, 0x00, 0x58, 0x1d, 0x7a, 0xbb,
			0x00, 0x0e, 0x08, 0x32, 0xc9, 0x57, 0x00, 0x00,
			0x00, 0x00, 0x97, 0xbc, 0x05, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
			0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
			0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
			0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
			0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
			0x36, 0x37
};
static void ip_pack_func(void** state) {
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;

// frist test : tcp packet
	Packet* packet = nic_alloc(__nics[0], 66);
	memcpy(packet->buffer + packet->start, sample_tcp_packet, sizeof(sample_tcp_packet));
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	
	uint16_t comp_len = ip->length;
	uint8_t comp_ttl = ip->ttl;
	uint16_t comp_checksum = ip->checksum;
	uint16_t comp_end = packet->end;

	/*	initialization	*/
	ip->length = 0;
	ip->ttl = 0;
	ip->checksum = 0;
	packet->end = 0;

	ip_pack(packet, 32);

	assert_int_equal(ip->length, comp_len);
	assert_int_equal(ip->ttl, comp_ttl);
	assert_int_equal(ip->checksum, comp_checksum);
	assert_int_equal(packet->end, comp_end);



// second test : icmp packet
	packet = nic_alloc(__nics[0], 98);
	memcpy(packet->buffer + packet->start, sample_icmp_packet, sizeof(sample_icmp_packet));

	
	ether = (Ether*)(packet->buffer + packet->start);
	ip = (IP*)ether->payload;
	
	comp_len = ip->length;
	comp_ttl = ip->ttl;
	comp_checksum = ip->checksum;
	comp_end = packet->end;

	/*	initialization	*/
	ip->length = 0;
	ip->ttl = 0;
	ip->checksum = 0;
	packet->end = 0;

	ip_pack(packet, 64);

	assert_int_equal(ip->length, comp_len);
	assert_int_equal(ip->ttl, comp_ttl);
	assert_int_equal(ip->checksum, comp_checksum);
	assert_int_equal(packet->end, comp_end);

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(ip_pack_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
