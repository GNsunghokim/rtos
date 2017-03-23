#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/ether.h>
#include <string.h>

#include <stdio.h>

#define CHAR_SIZE	26

/**
 * NOTE: There is opinion that this unit test is accomplished by another logic.
 *		so variable 'comp_buf' is calculated another logic and compared with the returned value of function.
 */

static char* buffer = "abcdefghijklmnopqrstuvwxyz";	// test string from 'a' to 'z' 

static void read_u8_func(void** state) {
	uint8_t comp_buf = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE; i++) {
		comp_buf = 'a' + i;
		assert_int_equal(read_u8(buffer, &idx), comp_buf);
	}
}
static void read_u16_func(void** state) {
	uint16_t comp_buf = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE; i += 2) {
		comp_buf = (('a' + i) << 8) | ('b' + i);
		assert_int_equal(read_u16(buffer, &idx), comp_buf);
	}
}
static void read_u32_func(void** state) {
	uint32_t comp_buf = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE - 2; i += 4) {
		comp_buf = (('a' + i) << 24) | (('b' + i) << 16) | (('c' + i) << 8) | ('d' + i);  
		assert_int_equal(read_u32(buffer, &idx), comp_buf);
	}
}
static void read_u48_func(void** state) {
	uint64_t comp_buf = 0;
	uint32_t idx = 0;

	for(uint64_t i = 0; i < CHAR_SIZE - 2; i += 6) {
		comp_buf = (('a' + i) << 40) | (('b' + i) << 32) | (('c' + i) << 24) | (('d' + i) << 16) | (('e' + i) << 8) | ('f' + i);  
		assert_int_equal(read_u48(buffer, &idx), comp_buf);
	}
}
static void read_u64_func(void** state) {
	uint64_t comp_buf = 0;
	uint32_t idx = 0;

	for(uint64_t i = 0; i < CHAR_SIZE - 2; i += 8) {
		comp_buf = (('a' + i) << 56) | (('b' + i) << 48) |  (('c' + i) << 40) | (('d' + i) << 32) | (('e' + i) << 24) | (('f' + i) << 16) | (('g' + i) << 8) | ('h' + i);  
		assert_int_equal(read_u64(buffer, &idx), comp_buf);
	}
}
static void read_string_func(void** state) {
	uint32_t idx = 0;

	for(size_t i = 0; i < CHAR_SIZE; i++) {
		char* comp_buf = read_string(buffer + i, &idx);
		idx = 0;
		
		assert_string_equal(&buffer[i], comp_buf);
	}
}
static void write_u8_func(void** state) {
	char buf[27];
	char comp_buf[27];
	uint8_t value = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE; i++) {
		value = 'a' + i;
		sprintf(comp_buf + i, "%c%c", 'a' + i, '\0');
		
		write_u8(buf, value, &idx);
		buf[idx] = '\0';
		assert_string_equal(buf, comp_buf); 
	}
}
static void write_u16_func(void** state) {
	char buf[27];
	char comp_buf[27];
	uint16_t value = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE; i += 2) {
		value = (('a' + i) << 8) | ('b' + i);
		sprintf(comp_buf + i, "%c%c%c", 'a' + i, 'b' + i, '\0');
		
		write_u16(buf, value, &idx);
		buf[idx] = '\0';
		assert_string_equal(buf, comp_buf); 
	}
}
static void write_u32_func(void** state) {
	char buf[27];
	char comp_buf[27];
	uint32_t value = 0;
	uint32_t idx = 0;

	for(uint32_t i = 0; i < CHAR_SIZE; i += 4) {
		value = (('a' + i) << 24) | (('b' + i) << 16) | (('c' + i) << 8) | ('d' + i);  
		sprintf(comp_buf + i, "%c%c%c%c%c", 'a' + i, 'b' + i, 'c' + i, 'd' + i, '\0');
		
		write_u32(buf, value, &idx);
		buf[idx] = '\0';
		assert_string_equal(buf, comp_buf); 
	}
}
static void write_u48_func(void** state) {
	char buf[27];
	char comp_buf[27];
	uint64_t value = 0;
	uint32_t idx = 0;

	for(uint64_t i = 0; i < CHAR_SIZE - 2; i += 6) {
		value = (('a' + i) << 40) | (('b' + i) << 32) | (('c' + i) << 24) | (('d' + i) << 16) | (('e' + i) << 8) | ('f' + i);  
		sprintf(comp_buf + i, "%c%c%c%c%c%c%c", 'a' + (int)i, 'b' + (int)i, 'c' + (int)i, 'd' + (int)i, 'e' + (int)i, 'f' + (int)i, '\0');
		
		write_u48(buf, value, &idx);
		buf[idx] = '\0';
		assert_string_equal(buf, comp_buf); 
	}
}
static void write_u64_func(void** state) {
	char buf[27];
	char comp_buf[27];
	uint64_t value = 0;
	uint32_t idx = 0;

	for(uint64_t i = 0; i < CHAR_SIZE - 2; i += 8) {
		value = (('a' + i) << 56) | (('b' + i) << 48) |  (('c' + i) << 40) | (('d' + i) << 32) | (('e' + i) << 24) | (('f' + i) << 16) | (('g' + i) << 8) | ('h' + i);  
		sprintf(comp_buf + i, "%c%c%c%c%c%c%c%c%c", 'a' + (int)i, 'b' + (int)i, 'c' + (int)i, 'd' + (int)i, 'e' + (int)i, 'f' + (int)i, 'g' + (int)i, 'h' + (int)i, '\0');
		
		write_u64(buf, value, &idx);
		buf[idx] = '\0';
		assert_string_equal(buf, comp_buf); 
	}	
}
static void write_string_func(void** state) {
	uint32_t idx = 0;
	char buf[27];
	char cpy_buf[8];

	for(size_t i = 0; i < CHAR_SIZE; i++) {
		sprintf(cpy_buf, "%c", 'a' + (int)i);
		write_string(buf, cpy_buf, &idx);
	}
	buf[26] = '\0';
	assert_string_equal(buffer, buf);
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(read_u8_func),
		cmocka_unit_test(read_u16_func),
		cmocka_unit_test(read_u32_func),
		cmocka_unit_test(read_u48_func),
		cmocka_unit_test(read_u64_func),
		cmocka_unit_test(read_string_func),
		cmocka_unit_test(write_u8_func),
		cmocka_unit_test(write_u16_func),
		cmocka_unit_test(write_u32_func),
		cmocka_unit_test(write_u48_func),
		cmocka_unit_test(write_u64_func),
		cmocka_unit_test(write_string_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
