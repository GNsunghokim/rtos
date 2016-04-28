#include <_string.h>
#include <lock.h>
#include <util/map.h>
#include <net/ni.h>
#include <net/interface.h>
#include <errno.h>
#include <_malloc.h>
#include <stdio.h>

int __nis_count;
NetworkInterface* __nis[NIS_SIZE];

#define ALIGN           16

inline int ni_count() {
	return __nis_count;
}

NetworkInterface* ni_get(int i) {
	if(i < __nis_count)
		return __nis[i];
	else
		return NULL;
}

Packet* ni_alloc(NetworkInterface* ni, uint16_t size) {
	Packet* packet = __malloc(sizeof(Packet) + size + 4 /* VLAN padding */ + ALIGN - 1, ni->pool);
	if(packet) {
		bzero(packet, sizeof(Packet));
		packet->ni = ni;
		packet->size = size + ALIGN - 1;
		packet->start = (((uintptr_t)packet->buffer + 4 /* VLAN padding */ + ALIGN - 1) & ~(ALIGN - 1)) - (uintptr_t)packet->buffer;
		packet->end = packet->start + size;
	}
	return packet;
}

inline void ni_free(Packet* packet) {
	__free(packet, packet->ni->pool);
}

inline bool ni_has_input(NetworkInterface* ni) {
	return !fifo_empty(ni->input_buffer);
}

inline Packet* ni_tryinput(NetworkInterface* ni) {
	if(lock_trylock(&ni->input_lock)) {
		Packet* packet = fifo_pop(ni->input_buffer);
		lock_unlock(&ni->input_lock);
		
		return packet;
	} else {
		return NULL;
	}
}

inline Packet* ni_input(NetworkInterface* ni) {
	lock_lock(&ni->input_lock);
	
	Packet* packet = fifo_pop(ni->input_buffer);
	
	lock_unlock(&ni->input_lock);
	
	return packet;
}

inline bool ni_output_available(NetworkInterface* ni) {
	return fifo_available(ni->output_buffer);
}

inline bool ni_has_output(NetworkInterface* ni) {
	return !fifo_empty(ni->output_buffer);
}

bool ni_output(NetworkInterface* ni, Packet* packet) {
	lock_lock(&ni->output_lock);
	
	if(fifo_push(ni->output_buffer, packet)) {
		lock_unlock(&ni->output_lock);
		return true;
	} else {
		ni->output_drop_bytes += packet->end - packet->start;
		ni->output_drop_packets++;
		
		lock_unlock(&ni->output_lock);
		
		ni_free(packet);
		
		return false;
	}
}

bool ni_output_dup(NetworkInterface* ni, Packet* packet) {
	uint16_t len = packet->end - packet->start;
	
	Packet* p = ni_alloc(ni, len);
	if(!p)
		return false;
	
	// Copy packet data
	uint16_t start = p->start;
	uint16_t size = p->size;
	
	memcpy(p, packet, sizeof(Packet));
	p->ni = ni;
	p->start = start;
	p->size = size;
	p->end = p->start + len;
	
	// Copy packet buffer
	memcpy(p->buffer + p->start, packet->buffer + packet->start, len);
	
	return ni_output(ni, p);
}

inline bool ni_tryoutput(NetworkInterface* ni, Packet* packet) {
	if(lock_trylock(&ni->output_lock)) {
		bool result = fifo_push(ni->output_buffer, packet);
		lock_unlock(&ni->output_lock);
		
		return result;
	} else {
		return false;
	}
}

size_t ni_pool_used(NetworkInterface* ni) {
	return __get_used_size(ni->pool);
}

size_t ni_pool_free(NetworkInterface* ni) {
	return __get_total_size(ni->pool) - __get_used_size(ni->pool);
}

size_t ni_pool_total(NetworkInterface* ni) {
	return __get_total_size(ni->pool);
}

#define CONFIG_INIT					\
if(ni->config->equals != map_string_equals) {		\
	ni->config->equals = map_string_equals;		\
	ni->config->hash = map_string_hash;		\
}

bool ni_config_put(NetworkInterface* ni, char* key, void* data) {
	CONFIG_INIT;
	
	if(map_contains(ni->config, key)) {
		map_update(ni->config, key, data);
	} else {
		int len = strlen(key) + 1;
		char* key2 = __malloc(len, ni->pool);
		if(key2 == NULL)
			return false;
		memcpy(key2, key, len);
		
		if(!map_put(ni->config, key2, data)) {
			__free(key2, ni->pool);
			return false;
		} else
			return true;
	}

	return true;
}

bool ni_config_contains(NetworkInterface* ni, char* key) {
	CONFIG_INIT;
	
	return map_contains(ni->config, key);
}

void* ni_config_remove(NetworkInterface* ni, char* key) {
	CONFIG_INIT;
	
	if(map_contains(ni->config, key)) {
		char* key2 = map_get_key(ni->config, key);
		void* data = map_remove(ni->config, key);
		__free(key2, ni->pool);
		
		return data;
	} else {
		return NULL;
	}
}

void* ni_config_get(NetworkInterface* ni, char* key) {
	CONFIG_INIT;
	
	return map_get(ni->config, key);
}

bool ni_ip_add(NetworkInterface* ni, uint32_t addr) {
	Map* interfaces = ni_config_get(ni, NI_ADDR_IPv4);
	if(!interfaces) {
		interfaces = map_create(16, NULL, NULL, ni->pool);
		ni_config_put(ni, NI_ADDR_IPv4, interfaces);
	}

	if(!interfaces)
		return false;

	IPv4Interface* interface = interface_alloc(ni->pool);
	if(!interface)
		return false;

	interface->netmask = 0xffffff00;
	interface->gateway = (addr & interface->netmask) | 0x1;
	interface->_default = true;

	if(!map_put(interfaces, (void*)(uintptr_t)addr, interface)) {
		interface_free(interface, ni->pool);
		return false;
	}

	return true;
}

IPv4Interface* ni_ip_get(NetworkInterface* ni, uint32_t addr) {
	Map* interfaces = ni_config_get(ni, NI_ADDR_IPv4);
	if(!interfaces)
		return NULL;

	return map_get(interfaces, (void*)(uintptr_t)addr);
}

bool ni_ip_remove(NetworkInterface* ni, uint32_t addr) {
	Map* interfaces = ni_config_get(ni, NI_ADDR_IPv4);
	if(!interfaces)
		return false;

	IPv4Interface* interface =  map_remove(interfaces, (void*)(uintptr_t)addr);
	if(!interface)
		return false;

	interface_free(interface, ni->pool);

	return true;
}
