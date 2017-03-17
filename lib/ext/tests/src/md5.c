/*
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/md5.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/md5.h>
#include <openssl/md5_locl.h>

#define ENTRY_CNT		14	//random value
#define MSG_SIZE		128	//random value
*/
/**
 * This function uses openssl library for comparing hash return values.
 */
/*
static const char* test_message[ENTRY_CNT] = { 
	"didfdfkfjalkdfkaldfjldjfkldsfklasjlkfajdskljdklafjdklfjkldajfkldfjdvf", 
	"d41d8ruvyh5",
	"a21rqfeasdzgw5trtarfAswehuy564teawgswt54rwfegthe6tkiytyeayh5vwyy", 
	"0cc175fdb9c0f1b6a831c399e2697726612134yhuetjreafaefaefaefaefaefaefaefaefaewsekijeezryhtuvj7",
	"abc123466iujkfhgcgfdasdfdfdfdfdfdfaefefefaewfyethugbgew4358u4tjrngsdbfhdgvskdeurerfvwt4", 
	"90015098df3cd24fb0d6963f7d28e17f722135t4rehtgdr65435tewgdjt6yrvtgf",
	"message digest123eefehgeth45324qrfsgs6t63qerjtesyr45tvtee", 
	"f96b697d7cb7938d525a2f5<F3>4y35htg6y4t23tfrryv45y",
	"abcdefghijklmnopqrstuvwxyz1354tyethyjf67uygt658uiekudre4r534vyrf", 
	"c3fcd3d761df92e4007dfb496cca67evdvadbrjtjufkufkthrgfgaefefeafave13",
	"ABCDEFGHIJKLdMNOPQRSTU",
	"d174ab98d277dfdfdfdfdf9f5a5611c2c9f419d9fasdasfaeghrfgyhqrwrqwrewfdsfvadh",
	"1234567890123401234567890123456789012345678901234567890v678", 
	"57edf4a22be3c955ac49da2e2107b67a"
};  

static void md5_func(void** state) {

	for(size_t i = 0; i < ENTRY_CNT; i++) {
		uint8_t* message = (uint8_t*)test_message[i];

		uint32_t hash[4];
		md5(message, strlen((char*)message), hash);

		// Openssl md5 hash creating
		uint8_t digest[MD5_DIGEST_LENGTH];
		uint32_t comp_hash[4];

		MD5_CTX context;
		MD5_Init(&context);
		MD5_Update(&context, message, strlen((char*)message));
		MD5_Final(digest, &context);

		memcpy(comp_hash, digest, MD5_DIGEST_LENGTH);

		// Comparing hash values
		for(int i = 0; i < 4; i++) { 
			assert_int_equal(hash[i], comp_hash[i]);
		}
	}
}

// 'md5_blocks_func' is not implemented.
static void md5_blocks_func(void** state) {
	void** blocks = (void**)&test_message2;
	uint32_t block_count = ENTRY_CNT;
	uint32_t block_size = MSG_SIZE;
	uint32_t hash[4];
	uint64_t len = 0;
	
	for(size_t i = 0; i < ENTRY_CNT; i++) {
		len += strlen((char*)test_message2[i]);
	}
	md5_blocks(blocks, block_count, block_size, len, hash);

	// Openssl md5 hash creating
	uint8_t digest[MD5_DIGEST_LENGTH];
	uint32_t comp_hash[4];

	MD5_CTX context;
	MD5_Init(&context);
	md5_block_data_order(&context, blocks[0], len);
	MD5_Final(digest, &context);

	memcpy(comp_hash, digest, MD5_DIGEST_LENGTH);

	for(int i = 0; i < 4; i++) { 
		assert_int_equal(hash[i], comp_hash[i]);
	}
}
int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(md5_func),
		cmocka_unit_test(md5_blocks_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}

*/
