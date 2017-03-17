#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "util/types.h"

bool is_uint8(const char* val) {
	char* end = NULL;
	long int v = strtol(val, &end, 0);

	if(end == NULL || *end != '\0')
		return false;

	if(v < 0 || v > UINT8_MAX)
		return false;
	return true;
}

uint8_t parse_uint8(const char* val) {
	return strtol(val, NULL, 0);
}

bool is_uint16(const char* val) {
	char* end = NULL;
	long int v = strtol(val, &end, 0);

	if(end == NULL || *end != '\0')
		return false;

	if(v < 0 || v > UINT16_MAX)
		return false;

	return true;
}

uint16_t parse_uint16(const char* val) {
	return strtol(val, NULL, 0);
}

bool is_uint32(const char* val) {
	char* end = NULL;
	long int v = strtol(val, &end, 0);

	if(end == NULL || *end != '\0')
		return false;

	if(v < 0 || v > UINT32_MAX)
		return false;
	return true;
}

uint32_t parse_uint32(const char* val) {
	return strtol(val, NULL, 0);
}

bool is_uint64(const char* val) {
	char* end = NULL;
	long int v = strtol(val, &end, 0);

	if(end == NULL || *end != '\0')
		return false;

	if(v < 0 || (uint64_t)v > UINT64_MAX)
		return false;

	return true;
}

uint64_t parse_uint64(const char* val) {
	return strtol(val, NULL, 0);
}
