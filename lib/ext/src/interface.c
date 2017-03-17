#include <net/interface.h>
#include <_malloc.h>
#include <string.h>

IPv4Interface* interface_alloc(void* pool) {
	size_t size = sizeof(IPv4Interface);
	IPv4Interface* interface = __malloc(size, pool);
	if(!interface)
		return NULL;

	bzero(interface, size);

	return interface;
}

void interface_free(IPv4Interface* interface, void* pool) {
	if(interface->udp_ports)
		set_destroy(interface->udp_ports);
	if(interface->tcp_ports)
		set_destroy(interface->tcp_ports);

	__free(interface, pool);
}
