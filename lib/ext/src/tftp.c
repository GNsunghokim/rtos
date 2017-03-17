#include <string.h>
#include <malloc.h>
#include <time.h>
#include <net/nic.h>
#include <net/interface.h>
#include <net/ether.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/tftp.h>
#include <util/map.h>

typedef struct {
	NIC*		nic;
	uint16_t	saddr;
	uint16_t	sport;
	
	uint32_t	daddr;
	uint16_t	dport;
	char*		filename;
	char*		mode;
	uint16_t	ack;
	clock_t		time;
	
	int		opcode;	// 1: read, 2: write
	int		fd;
	uint32_t	offset;
} Session;

static clock_t next_gc;

uint32_t TFTP_SESSION_GC = (5 * 1000000);	// 5 secs
uint32_t TFTP_SESSION_TTL = (10 * 1000000);	// 10 secs

//static clock_t last_gc;
static Map* sessions;

static void delete(Session* session) {
	udp_port_free(session->nic, session->saddr, session->sport);
	free(session->filename);
	free(session->mode);
	free(session);
}

static Session* create(Packet* packet, IP* ip, UDP* udp, TFTPCallback* callback) {
	uint32_t index = 0;
	uint16_t opcode = read_u16(udp->body, &index);
	char* filename = read_string(udp->body, &index);
	index++;
	char* mode = read_string(udp->body, &index);
	index++;
	
	Session* session = malloc(sizeof(Session));
	bzero(session, sizeof(Session));
	session->nic = packet->nic;
	session->saddr = endian32(ip->destination);
	session->sport = udp_port_alloc(session->nic, session->saddr);
	session->daddr = endian32(ip->source);
	session->dport = endian16(udp->source);
	int len = strlen(filename) + 1;
	session->filename = malloc(len);
	memcpy(session->filename, filename, len);
	len = strlen(mode) + 1;
	session->mode = malloc(len);
	memcpy(session->mode, mode, len);
	session->ack = 0;
	session->time = clock() + TFTP_SESSION_TTL;
	
	session->opcode= opcode;
	session->fd = callback->create(session->filename, opcode);
	if(session->fd < 0) {
		delete(session);
		return NULL;
	}
	session->offset = 0;
	
	if(!sessions) {
		sessions = map_create(4, map_uint64_hash, map_uint64_equals, NULL);
	}
	
	map_put(sessions, (void*)(uintptr_t)session->sport, session);
	
	return session;
}

static void close(Session* session) {
	map_remove(sessions, (void*)(uintptr_t)session->sport);
	delete(session);
}

static void ack(Packet* packet, Session* session) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)ip->body;
	
	uint32_t index = 0;
	write_u16(udp->body, 4, &index);
	write_u16(udp->body, session->ack++, &index);
	
	udp->destination = endian16(session->dport);
	udp->source = endian16(session->sport);
	swap32(ip->source, ip->destination);
	swap48(ether->smac, ether->dmac);
	
	udp_pack(packet, index);
	
	nic_output(packet->nic, packet);
}

static void nack(Packet* packet, uint16_t error_number, char* error_message) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)ip->body;
	
	uint32_t index = 0;
	write_u16(udp->body, 5, &index);
	write_u16(udp->body, error_number, &index);
	write_string(udp->body, error_message, &index);
	udp->body[index++] = 0;
	
	swap16(udp->source, udp->destination);
	swap32(ip->source, ip->destination);
	swap48(ether->smac, ether->dmac);
	
	udp_pack(packet, index);
	
	nic_output(packet->nic, packet);
}

bool tftp_process(Packet* packet) {
	// GC
	clock_t time = clock();
	if(sessions && next_gc < time) {
		MapIterator iter;
		map_iterator_init(&iter, sessions);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			Session* session = entry->data;
			if(session->time < time) {
				map_iterator_remove(&iter);
				delete(session);
			}
		}
		
		next_gc = time + TFTP_SESSION_GC;
	} 
	
	TFTPCallback* callback = nic_config_get(packet->nic, TFTP_CALLBACK); 
	if(!callback)
		return false;
	
	// Parse packet
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_IPv4)
		return false;
	
	IP* ip = (IP*)ether->payload;
	if(ip->protocol != IP_PROTOCOL_UDP)
		return false;

	uint32_t addr = endian32(ip->destination);
	IPv4Interface* interface = nic_ip_get(packet->nic, addr);
	if(!interface)
		return false;

	udp_port_alloc0(packet->nic, addr, 69);
	uint16_t port = 69;
	
	UDP* udp = (UDP*)ip->body;
	Session* session;
	uint16_t dport = endian16(udp->destination);
	if(dport == port) {
		uint32_t index = 0;
		uint16_t opcode = read_u16(udp->body, &index);
		switch(opcode) {
			case 1: // Read request (RRQ)
			case 2: // Write request (WRQ)
				session = create(packet, ip, udp, callback);
				if(session) {
					ack(packet, session);
				} else {
					nack(packet, 2, "There is no such storage");
				}
				return true;
			default:
				nic_free(packet);
				printf("Unsupported opcode: %d\n", opcode);
				return true;
		}
	} else if(sessions && (session = map_get(sessions, (void*)(uintptr_t)dport))) {
		session->time = time + TFTP_SESSION_TTL;
		
		uint32_t index = 0;
		uint16_t opcode = read_u16(udp->body, &index);
		switch(opcode) {
			case 3: // Data (DATA)
				;
				uint16_t block_number = read_u16(udp->body, &index);
				if(block_number != session->ack) {
					ack(packet, session);
					return true;
				}
				
				int size = endian16(udp->length) - UDP_LEN - index;
				if(callback->write(session->fd, udp->body + index, (block_number - 1) * 512, size) != size) {
					nack(packet, 3, "Disk full");
					
					close(session);
				} else {
					ack(packet, session);
					
					if(size < 512) {
						close(session);
					}
				}
				return true;
			case 4: // Acknowledgment (ACK)
			case 5: // Error (ERROR)
			default:
				close(session);
				nic_free(packet);
				printf("Unsupported opcode: %d\n", opcode);
				return true;
		}
	}
	
	return false;
}
