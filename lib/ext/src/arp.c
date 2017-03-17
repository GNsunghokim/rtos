#include <timer.h>
#include <_malloc.h>
#include <net/interface.h>
#include <net/ether.h>
#include <net/arp.h>
#include <util/map.h>

#define ARP_TABLE	"net.arp.arptable"
#define ARP_TABLE_GC	"net.arp.arptable.gc"

#define GC_INTERVAL	(10 * 1000000)	// 10 secs

typedef struct {
	uint64_t	mac;
	uint64_t	timeout;
} ARPEntity;

uint64_t ARP_TIMEOUT = (uint64_t)14400 * 1000000;	// 4 hours  (us)

bool arp_process(Packet* packet) {
#ifndef	LINUX
	extern uint64_t __timer_ms;
	if(!__timer_ms)
		return false;
#endif

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_ARP)
		return false;

	ARP* arp = (ARP*)ether->payload;
	uint32_t addr = endian32(arp->tpa);

	if(!nic_ip_get(packet->nic, addr))
		return false;

	Map* arp_table = nic_config_get(packet->nic, ARP_TABLE);
	if(!arp_table) {
		arp_table = map_create(32, map_uint64_hash, map_uint64_equals, packet->nic->pool);
		if(!arp_table)
			return false;
		if(!nic_config_put(packet->nic, ARP_TABLE, arp_table)) {
			map_destroy(arp_table);
			return false;
		}
	}

	uint64_t current = timer_us();

	// GC
	uint64_t gc_time = (uintptr_t)nic_config_get(packet->nic, ARP_TABLE_GC);
	if(gc_time == 0 && !nic_config_contains(packet->nic, ARP_TABLE_GC)) {
		gc_time = current + GC_INTERVAL;
		if(!nic_config_put(packet->nic, ARP_TABLE_GC, (void*)(uintptr_t)gc_time))
			return false;
	}

	if(gc_time < current) {
		MapIterator iter;
		map_iterator_init(&iter, arp_table);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			if(((ARPEntity*)entry->data)->timeout < current) {
				__free(entry->data, packet->nic->pool);
				map_iterator_remove(&iter);
			}
		}

		gc_time = current + GC_INTERVAL;
		if(!nic_config_put(packet->nic, ARP_TABLE_GC, (void*)(uintptr_t)gc_time))
			return false;
	}

	switch(endian16(arp->operation)) {
		case 1:	// Request
			ether->dmac = ether->smac;
			ether->smac = endian48(packet->nic->mac);
			arp->operation = endian16(2);
			arp->tha = arp->sha;
			arp->tpa = arp->spa;
			arp->sha = ether->smac;
			arp->spa = endian32(addr);

			nic_output(packet->nic, packet);

			return true;
		case 2: // Reply
			;
			uint64_t smac = endian48(arp->sha);
			uint32_t sip = endian32(arp->spa);
			ARPEntity* entity = map_get(arp_table, (void*)(uintptr_t)sip);
			if(!entity) {
				entity = __malloc(sizeof(ARPEntity), packet->nic->pool);
				if(!entity)
					goto done;

				if(!map_put(arp_table, (void*)(uintptr_t)sip, entity)) {
					__free(entity, packet->nic->pool);
					goto done;
				}

			}
			entity->mac = smac;
			entity->timeout = current + ARP_TIMEOUT;

done:
			nic_free(packet);
			return true;
	}

	return false;
}

bool arp_request(NIC* nic, uint32_t destination, uint32_t source) {
	if(source == 0) {
		Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
		if(!interfaces)
			return false;

		bool result = true;

		MapIterator iter;
		map_iterator_init(&iter, interfaces);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			source = (uint32_t)(uintptr_t)(entry->key);
			result &= arp_request(nic, destination, source);
		}
		return result;
	} else {
		Packet* packet = nic_alloc(nic, sizeof(Ether) + sizeof(ARP));
		if(!packet)
			return false;

		Ether* ether = (Ether*)(packet->buffer + packet->start);
		ether->dmac = endian48(0xffffffffffff);
		ether->smac = endian48(nic->mac);
		ether->type = endian16(ETHER_TYPE_ARP);

		ARP* arp = (ARP*)ether->payload;
		arp->htype = endian16(1);
		arp->ptype = endian16(0x0800);
		arp->hlen = endian8(6);
		arp->plen = endian8(4);
		arp->operation = endian16(1);
		arp->sha = endian48(nic->mac);
		arp->spa = endian32(source);
		arp->tha = endian48(0);
		arp->tpa = endian32(destination);

		packet->end = packet->start + sizeof(Ether) + sizeof(ARP);

		return nic_output(nic, packet);
	}
}

bool arp_announce(NIC* nic, uint32_t ip) {
	if(ip == 0) {
		Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
		if(!interfaces)
			return false;

		bool result = false;

		MapIterator iter;
		map_iterator_init(&iter, interfaces);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			ip = (uint32_t)(uintptr_t)entry->key;
			result &= arp_announce(nic, ip);
		}

		return result;
	} else {
		Packet* packet = nic_alloc(nic, sizeof(Ether) + sizeof(ARP));
		if(!packet)
			return false;

		Ether* ether = (Ether*)(packet->buffer + packet->start);
		ether->dmac = endian48(0xffffffffffff);
		ether->smac = endian48(nic->mac);
		ether->type = endian16(ETHER_TYPE_ARP);

		ARP* arp = (ARP*)ether->payload;
		arp->htype = endian16(1);
		arp->ptype = endian16(0x0800);
		arp->hlen = endian8(6);
		arp->plen = endian8(4);
		arp->operation = endian16(2);
		arp->sha = endian48(nic->mac);
		arp->spa = endian32(ip);
		arp->tha = endian48(0);
		arp->tpa = endian32(ip);

		packet->end = packet->start + sizeof(Ether) + sizeof(ARP);

		return nic_output(nic, packet);
	}
}

uint64_t arp_get_mac(NIC* nic, uint32_t destination, uint32_t source) {
	Map* arp_table = nic_config_get(nic, ARP_TABLE);
	if(!arp_table) {
		arp_request(nic, destination, source);
		return 0xffffffffffff;
	}

	ARPEntity* entity = map_get(arp_table, (void*)(uintptr_t)destination);
	if(!entity) {
		arp_request(nic, destination, source);
		return 0xffffffffffff;
	}

	return entity->mac;
}

uint32_t arp_get_ip(NIC* nic, uint64_t mac) {
	Map* arp_table = nic_config_get(nic, ARP_TABLE);
	if(!arp_table) {
		return 0;
	}

	MapIterator iter;
	map_iterator_init(&iter, arp_table);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		if(((ARPEntity*)entry->data)->mac == mac)
			return (uint32_t)(uintptr_t)entry->key;
	}

	return 0;
}

void arp_pack(Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ARP* arp = (ARP*)ether->payload;

	packet->end = ((void*)arp + sizeof(ARP)) - (void*)packet->buffer;
}
