/* 
 * MD5 hash in C and x86 assembly
 * 
 * Copyright (c) 2012 Nayuki Minase. All rights reserved.
 * http://nayuki.eigenstate.org/page/fast-md5-hash-implementation-in-x86-assembly
 */

#include <string.h>
#include <net/md5.h>

void md5_compress(uint32_t *state, uint32_t *block);

void md5(uint8_t* message, uint32_t len, uint32_t* hash) {
	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;

	uint32_t i;
	for(i = 0; i + 64 <= len; i += 64) 
		md5_compress(hash, (uint32_t*)(message + i));

	uint32_t block[16];
	uint8_t *byteBlock = (uint8_t*)block;
	int rem = len - i;
	memcpy(byteBlock, message + i, rem);

	byteBlock[rem] = 0x80;
	rem++;
	if(64 - rem >= 8) {
		memset(byteBlock + rem, 0, 56 - rem);
	} else {
		memset(byteBlock + rem, 0, 64 - rem);
		md5_compress(hash, block);
		memset(block, 0, 56);
	}

	block[14] = len << 3;
	block[15] = len >> 29;
	md5_compress(hash, block);
}

void md5_blocks(void** blocks, uint32_t block_count, uint32_t block_size, uint64_t len, uint32_t* hash) {
	uint64_t len0 = len;
	
	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;
	
	uint8_t* message = blocks[0];
	uint32_t j = 0;
	for(uint32_t i = 0; i < block_count; i++) {
		message = blocks[i];
		for(j = 0; j + 64 <= block_size && len >= 64; j += 64, len -= 64) {
			md5_compress(hash, (uint32_t*)(message + j));
		}
		
		if(len < 64)
			break;
	}
	uint32_t block[16];
	uint8_t *byteBlock = (uint8_t*)block;
	int rem = len;
	memcpy(byteBlock, message + j, rem);

	byteBlock[rem] = 0x80;
	rem++;
	if(64 - rem >= 8) {
		memset(byteBlock + rem, 0, 56 - rem);
	} else {
		memset(byteBlock + rem, 0, 64 - rem);
		md5_compress(hash, block);
		memset(block, 0, 56);
	}
	
	block[14] = len0 << 3;
	block[15] = len0 >> 29;
	md5_compress(hash, block);
}
