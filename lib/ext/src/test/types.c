#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>		//snprintf
#include <util/types.h>

#define ARY_SIZE 16
#define STR_SIZE 20

static uint64_t test_values[] = { 0x0, 0xf, 0x10, 0xff, 0x100, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xffffffff, 0x100000000, 0xfffffffffffffff, 0x1000000000000000, 0x7fffffffffffffff, 0x8000000000000000, 0xffffffffffffffff };
static char integer_str[ARY_SIZE][STR_SIZE];

/* This function is definition of test cases for utin#_func.
 * Each string has minimun and maximum  hax values of 4bit, 8bit, 16bit, 32bit and 64bit.
 *
 * NOTE: "parse_uint#" function has a constraint about maximum. If the hax value is over 0x8000000000000000(16bit), all of returned values fixed 0x7fffffffffffffff(16bit).
 * This is check point whether to be right or wrong.
 */
static void set_integer_str() {
	snprintf(integer_str[0], STR_SIZE, "0x0");
	snprintf(integer_str[1], STR_SIZE, "0xf");
	snprintf(integer_str[2], STR_SIZE, "0x10");
	snprintf(integer_str[3], STR_SIZE, "0xff");
	snprintf(integer_str[4], STR_SIZE, "0x100");
	snprintf(integer_str[5], STR_SIZE, "0xffff");
	snprintf(integer_str[6], STR_SIZE, "0x10000");
	snprintf(integer_str[7], STR_SIZE, "0x7fffffff");
	snprintf(integer_str[8], STR_SIZE, "0x80000000");
	snprintf(integer_str[9], STR_SIZE, "0xffffffff");
	snprintf(integer_str[10], STR_SIZE, "0x100000000");
	snprintf(integer_str[11], STR_SIZE, "0xfffffffffffffff");
	snprintf(integer_str[12], STR_SIZE, "0x1000000000000000");
	snprintf(integer_str[13], STR_SIZE, "0x7fffffffffffffff");
	snprintf(integer_str[14], STR_SIZE, "0x8000000000000000");
	snprintf(integer_str[15], STR_SIZE, "0xffffffffffffffff");
}

static void is_uint8_func(void** state) {
	for(int i = 0; i < 4; i++)
		assert_true(is_uint8(integer_str[i]));	
	for(int i = 4; i < ARY_SIZE; i++)
		assert_false(is_uint8(integer_str[i]));
}

static void parse_uint8_func(void** state) {
	for(int i = 0; i < ARY_SIZE; i++) {
		if(is_uint8(integer_str[i])) {
			assert_int_equal(parse_uint8(integer_str[i]), test_values[i]);
		}
	}
}

static void is_uint16_func(void** state) {
	for(int i = 0; i < 6; i++)
		assert_true(is_uint16(integer_str[i]));	
	for(int i = 6; i < ARY_SIZE; i++)
		assert_false(is_uint16(integer_str[i]));
}

static void parse_uint16_func(void** state) {
	for(int i = 0; i < ARY_SIZE; i++) {
		if(is_uint16(integer_str[i])) {
			assert_int_equal(parse_uint16(integer_str[i]), test_values[i]);
		}
	}
}

static void is_uint32_func(void** state) {
	for(int i = 0; i < 10; i++)
		assert_true(is_uint32(integer_str[i]));	
	for(int i = 10; i < ARY_SIZE; i++)
		assert_false(is_uint32(integer_str[i]));
}

static void parse_uint32_func(void** state) {
	for(int i = 0; i < ARY_SIZE; i++) {
		if(is_uint32(integer_str[i])) {
			assert_int_equal(parse_uint32(integer_str[i]), test_values[i]);
		}
	}
}

static void is_uint64_func(void** state) {
	for(int i = 0; i < ARY_SIZE; i++)
		assert_true(is_uint64(integer_str[i]));	
}


static void parse_uint64_func(void** state) {
	for(int i = 0; i < ARY_SIZE; i++) {
		if(is_uint64(integer_str[i])) {
			assert_int_equal(parse_uint64(integer_str[i]), test_values[i]);
		}
	}
}


int main(void) {
	set_integer_str();
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(is_uint8_func),
		cmocka_unit_test(parse_uint8_func),
		cmocka_unit_test(is_uint16_func),
		cmocka_unit_test(parse_uint16_func),
		cmocka_unit_test(is_uint32_func),
		cmocka_unit_test(parse_uint32_func),
		cmocka_unit_test(is_uint64_func),
		cmocka_unit_test(parse_uint64_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
