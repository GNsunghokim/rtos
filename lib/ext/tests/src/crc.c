#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/crc.h>

#define DATA_LEN	100
/* 
 * Charles Michael Heard's CRC-32 Code                                  
 * 
 * crc32h.c -- package to compute 32-bit CRC one byte at a time using   
 *             the high-bit first (Big-Endian) bit ordering convention  
 *                                                                      
 * Synopsis:                                                            
 *  gen_crc_table() -- generates a 256-word table containing all CRC    
 *                     remainders for every possible 8-bit byte.  It    
 *                     must be executed (once) before any CRC updates.  
 *                                                                      
 *  unsigned update_crc(crc_accum, data_blk_ptr, data_blk_size)         
 *           unsigned crc_accum; char *data_blk_ptr; int data_blk_size; 
 *           Returns the updated value of the CRC accumulator after     
 *           processing each byte in the addressed block of data.      
 *
 *  The table lookup technique was adapted from the algorithm described 
 *  by Avram Perez, Byte-wise CRC Calculations, IEEE Micro 3, 40 (1983).
 *
 */

#define POLYNOMIAL	0x04c11db7L

static uint32_t crc_table[256];

/* generate the table of CRC remainders for all possible bytes */
void gen_crc_table()
{
	register int i, j;
	register uint32_t crc_accum;

	for(i=0; i<256; i++)
	{
		crc_accum = ((uint32_t) i << 24);
		for(j=0; j<8; j++)
		{
			if(crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			else
				crc_accum = (crc_accum << 1);
		}
		crc_table[i] = crc_accum;
	}
	return;
}

/* update the CRC on the data block one byte at a time */
unsigned int update_crc(uint32_t crc_accum, uint8_t *data_blk_ptr, uint32_t data_blk_size)
{
	register uint32_t i, j;

	for(j=0; j<data_blk_size; j++)
	{
		i = ((int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}
	return crc_accum;
}

/**
 * NOTE: Mock is needed for checking that rtn value is right return value of crc32 func.
 *		comp_rtn takes return value of mock function.
 *		this test is using mock through copying another crc soruce code.
 */


static void crc32_func(void** state) {
	uint8_t data[DATA_LEN];	//random value
	for(size_t i = 0; i < DATA_LEN; i++)
		data[i] = i;

	uint32_t length = DATA_LEN;

	gen_crc_table();

	uint32_t rtn = crc32(data, length);
	uint32_t comp_rtn = update_crc(0xffffffff, data, length) ^ 0xffffffff;
	assert_int_equal(rtn, comp_rtn);

}

static void crc32_update_func(void** state) {
	uint8_t data[DATA_LEN];	//random value
	for(size_t i = 0; i < DATA_LEN; i++)
		data[i] = i;

	uint32_t length = DATA_LEN;

	uint32_t rtn = crc32_update(0xffffffff, data, length);
	uint32_t comp_rtn = update_crc(0xffffffff, data, length);
	assert_int_equal(rtn, comp_rtn);

}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(crc32_func),
		cmocka_unit_test(crc32_update_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
