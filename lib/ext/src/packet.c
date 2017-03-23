#include <stdio.h>
#include <net/packet.h>

void packet_dump(Packet* packet) {
	uint8_t* p = packet->buffer + packet->start;
	int size = packet->end - packet->start;
	
	int i;
	for(i = 0; i < size; i++) {
		printf("%02x", p[i]);
		
		if((i + 1) % 16 != 0)
			printf(" ");
		else
			printf("\n");
	}
	
	if(i % 16 != 0)
		printf("\n");
}
