#ifndef __NET_ARP_H__
#define __NET_ARP_H__

#include <stdint.h>
#include <stdbool.h>
#include <nic.h>

/**
 * @file
 * ARP header and related functions
 */

#define ARP_LEN		28		///< ARP header length

/**
 * ARP header for IPv4
 */
typedef struct _ARP {
	uint16_t htype;			///< H/W Type (endian16)
	uint16_t ptype;			///< Protocol Type (endian16)
	uint8_t hlen;			///< H/W address length
	uint8_t plen;			///< Protocol address length
	uint16_t operation;		///< Operation (endian16)
	uint64_t sha: 48;		///< Sender H/W address (endian48)
	uint32_t spa;			///< Sender protocol address (endian32)
	uint64_t tha: 48;		///< Target H/W address (endian48)
	uint32_t tpa;			///< Target protocol address (endian32)
} __attribute__ ((packed)) ARP;

extern uint64_t ARP_TIMEOUT;		///< ARP timeout of ARP table

typedef struct _ARPEntity {
	uint64_t	mac;
	uint32_t	addr;
	uint64_t	timeout;

	bool		dynamic;
} ARPEntity;

#define ARP_ENTITY_MAX_COUNT	16
typedef struct ARPTable {
	uint64_t	timeout;
	uint16_t	entity_count;
	ARPEntity 	entity[ARP_ENTITY_MAX_COUNT];
} ARPTable;

/**
 * Get ARP Table
 *
 * @param NIC
 * @return ARP Table
 */
ARPTable* arp_get_table(NIC* nic);

#define ARP_PROCESS()
/**
 * Process ARP packet.
 *
 * @param packet the packet to process
 * @return ture if ARP packet is processed (packet must not be reused)
 */
bool arp_process(NIC* nic, Packet* packet);

/**
 * Broadcast ARP request (MAC address resolving).
 *
 * @param nic NIC reference to send ARP request
 * @param destination IP address to resolve MAC address
 * @param source IP address of interface
 * @return true if ARP request is sent
 */
bool arp_request0(NIC* nic, uint32_t destination, uint32_t source);
/**
 * Broadcast ARP request (MAC address resolving).
 *
 * @param nic NIC reference to send ARP request
 * @param destination IP address to resolve MAC address
 * @return true if ARP request is sent
 */
bool arp_request(NIC* nic, uint32_t destination);
/**
 * Announce IP and MAC address to hosts in the LAN.
 *
 * @param nic NIC reference to send ARP announce
 * @param ip IP address to announce
 * @return true if ARP announance is sent
 */
bool arp_announce(NIC* nic, uint32_t ip);

/**
 * Get MAC address associated with IP address from local ARP table.
 * It will return MAC address if there is an entity in ARP table,
 * it will return 0xffffffffffff if there is no entity in ARP table.
 * If there is no entity, ARP request is will be sent.
 *
 * @param nic NIC reference which manages ARP table
 * @param destination IP address
 * @param source IP address of interface
 * @return MAC address if there is entry in ARP table, if not 0xffffffffffff will be returned
 */
uint64_t arp_get_mac(NIC* nic, uint32_t destination, uint32_t source);

/**
 * Get IP address associated with MAC address from local ARP table.
 * 
 * @param nic NIC reference which manages ARP table
 * @param mac MAC address
 * @return IP address if there is entry in ARP table, if not 0.0.0.0 will be returned
 */
uint32_t arp_get_ip(NIC* nic, uint64_t mac);

/**
 * Set the packet->end index to end of ARP payload
 * @param packet ARP packet
 */
void arp_pack(Packet* packet);

#endif /* __NET_ARP_H__ */
