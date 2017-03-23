#include <tlsf.h>
#include <lock.h>
#include <util/map.h>
#include <net/nic.h>
#include <net/interface.h>
#include <errno.h>
#include <string.h>
#include <_malloc.h>

int __nic_count;
NIC* __nics[NIC_SIZE];

#define ALIGN           16

inline int nic_count() {
	return __nic_count;
}

NIC* nic_get(int i) {
	if(i < __nic_count)
		return __nics[i];
	else
		return NULL;
}

Packet* nic_alloc(NIC* nic, uint16_t size) {
	Packet* packet = __malloc(sizeof(Packet) + size + 4 /* VLAN padding */ + ALIGN - 1, nic->pool);
	if(packet) {
		bzero(packet, sizeof(Packet));
		packet->nic = nic;
		packet->size = size + ALIGN - 1;
		packet->start = (((uintptr_t)packet->buffer + 4 /* VLAN padding */ + ALIGN - 1) & ~(ALIGN - 1)) - (uintptr_t)packet->buffer;
		packet->end = packet->start + size;
	}
	
	return packet;
}

inline void nic_free(Packet* packet) {
	__free(packet, packet->nic->pool);
}

inline bool nic_has_input(NIC* nic) {
	return !fifo_empty(nic->input_buffer);
}

inline Packet* nic_tryinput(NIC* nic) {
	if(lock_trylock(&nic->input_lock)) {
		Packet* packet = fifo_pop(nic->input_buffer);
		lock_unlock(&nic->input_lock);
		
		return packet;
	} else {
		return NULL;
	}
}

inline Packet* nic_input(NIC* nic) {
	lock_lock(&nic->input_lock);
	
	Packet* packet = fifo_pop(nic->input_buffer);
	
	lock_unlock(&nic->input_lock);
	
	return packet;
}

inline bool nic_output_available(NIC* nic) {
	return fifo_available(nic->output_buffer);
}

inline bool nic_has_output(NIC* nic) {
	return !fifo_empty(nic->output_buffer);
}

bool nic_output(NIC* nic, Packet* packet) {
	lock_lock(&nic->output_lock);
	
	if(fifo_push(nic->output_buffer, packet)) {
		lock_unlock(&nic->output_lock);
		return true;
	} else {
		nic->output_drop_bytes += packet->end - packet->start;
		nic->output_drop_packets++;
		
		lock_unlock(&nic->output_lock);
		
		nic_free(packet);
		
		return false;
	}
}

bool nic_output_dup(NIC* nic, Packet* packet) {
	uint16_t len = packet->end - packet->start;
	
	Packet* p = nic_alloc(nic, len);
	if(!p)
		return false;
	
	// Copy packet data
	uint16_t start = p->start;
	uint16_t size = p->size;
	
	memcpy(p, packet, sizeof(Packet));
	p->nic = nic;
	p->start = start;
	p->size = size;
	p->end = p->start + len;
	
	// Copy packet buffer
	memcpy(p->buffer + p->start, packet->buffer + packet->start, len);
	
	return nic_output(nic, p);
}

inline bool nic_tryoutput(NIC* nic, Packet* packet) {
	if(lock_trylock(&nic->output_lock)) {
		bool result = fifo_push(nic->output_buffer, packet);
		lock_unlock(&nic->output_lock);
		
		return result;
	} else {
		return false;
	}
}

size_t nic_pool_used(NIC* nic) {
	return get_used_size(nic->pool);
}

size_t nic_pool_free(NIC* nic) {
	return get_total_size(nic->pool) - get_used_size(nic->pool);
}

size_t nic_pool_total(NIC* nic) {
	return get_total_size(nic->pool);
}

#define CONFIG_INIT					\
if(nic->config->equals != map_string_equals) {		\
	nic->config->equals = map_string_equals;		\
	nic->config->hash = map_string_hash;		\
}

bool nic_config_put(NIC* nic, char* key, void* data) {
	CONFIG_INIT;
	
	if(map_contains(nic->config, key)) {
		map_update(nic->config, key, data);
	} else {
		int len = strlen(key) + 1;
		char* key2 = __malloc(len, nic->pool);
		if(key2 == NULL)
			return false;
		memcpy(key2, key, len);
		
		if(!map_put(nic->config, key2, data)) {
			__free(key2, nic->pool);
			return false;
		} else
			return true;
	}

	return true;
}

bool nic_config_contains(NIC* nic, char* key) {
	CONFIG_INIT;
	
	return map_contains(nic->config, key);
}

void* nic_config_remove(NIC* nic, char* key) {
	CONFIG_INIT;
	
	if(map_contains(nic->config, key)) {
		char* key2 = map_get_key(nic->config, key);
		void* data = map_remove(nic->config, key);
		__free(key2, nic->pool);
		
		return data;
	} else {
		return NULL;
	}
}

void* nic_config_get(NIC* nic, char* key) {
	CONFIG_INIT;
	
	return map_get(nic->config, key);
}

bool nic_ip_add(NIC* nic, uint32_t addr) {
	Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
	if(!interfaces) {
		interfaces = map_create(16, NULL, NULL, nic->pool);
		nic_config_put(nic, NIC_ADDR_IPv4, interfaces);
	}

	if(!interfaces)
		return false;

	IPv4Interface* interface = interface_alloc(nic->pool);
	if(!interface)
		return false;

	interface->netmask = 0xffffff00;
	interface->gateway = (addr & interface->netmask) | 0x1;
	interface->_default = true;

	if(!map_put(interfaces, (void*)(uintptr_t)addr, interface)) {
		interface_free(interface, nic->pool);
		return false;
	}

	return true;
}

IPv4Interface* nic_ip_get(NIC* nic, uint32_t addr) {
	Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
	if(!interfaces)
		return NULL;

	return map_get(interfaces, (void*)(uintptr_t)addr);
}

bool nic_ip_remove(NIC* nic, uint32_t addr) {
	Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
	if(!interfaces)
		return false;

	IPv4Interface* interface =  map_remove(interfaces, (void*)(uintptr_t)addr);
	if(!interface)
		return false;

	interface_free(interface, nic->pool);

	return true;
}
