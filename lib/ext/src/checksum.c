#include <byteswap.h>
#include <net/checksum.h>

uint16_t checksum(void* data, uint32_t size) {
	uint32_t sum = 0;
	uint16_t* p = data;
	
	while(size > 1) {
		sum += *p++;
		if(sum >> 16)
			sum = (sum & 0xffff) + (sum >> 16);
		
		size -= 2;
	}
	
	if(size)
		sum += *(uint8_t*)p;
	
	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	
	return bswap_16((uint16_t)~sum);
}
