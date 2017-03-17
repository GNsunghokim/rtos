#include <net/ether.h>

uint8_t read_u8(void* buf, uint32_t* idx) {
	uint8_t value = *(uint8_t*)(buf + *idx);
	*idx += 1;
	
	return value;
}

uint16_t read_u16(void* buf, uint32_t* idx) {
	uint16_t value = *(uint16_t*)(buf + *idx);
	*idx += 2;
	
	return endian16(value);
}

uint32_t read_u32(void* buf, uint32_t* idx) {
	uint32_t value = *(uint32_t*)(buf + *idx);
	*idx += 4;
	
	return endian32(value);
}

uint64_t read_u48(void* buf, uint32_t* idx) {
	uint64_t value = *(uint64_t*)(buf + *idx);
	*idx += 6;
	
	return endian48(value);
}

uint64_t read_u64(void* buf, uint32_t* idx) {
	uint64_t value = *(uint64_t*)(buf + *idx);
	*idx += 8;
	
	return endian64(value);
}

char* read_string(void* buf, uint32_t* idx) {
	char* str = buf + *idx;
	char* p = str;
	while(*p != '\0') {
		*idx += 1;
		p++;
	}
	
	return str;
}

void write_u8(void* buf, uint8_t value, uint32_t* idx) {
	*(uint8_t*)(buf + *idx) = value;
	*idx += 1;
}

void write_u16(void* buf, uint16_t value, uint32_t* idx) {
	*(uint16_t*)(buf + *idx) = endian16(value);
	*idx += 2;
}

void write_u32(void* buf, uint32_t value, uint32_t* idx) {
	*(uint32_t*)(buf + *idx) = endian32(value);
	*idx += 4;
}

void write_u48(void* buf, uint64_t value, uint32_t* idx) {
	void* p = &value;
	*(uint16_t*)(buf + *idx) = endian16(*(uint16_t*)(p + 4));
	*(uint32_t*)(buf + *idx + 2) = endian32(*(uint32_t*)(p));
	*idx += 6;
}

void write_u64(void* buf, uint64_t value, uint32_t* idx) {
	*(uint64_t*)(buf + *idx) = endian64(value);
	*idx += 8;
}

void write_string(void* buf, const char* str, uint32_t* idx) {
	char* p = buf + *idx;
	while(*str != '\0') {
		*idx += 1;
		*p++ = *str++;
	}
}
