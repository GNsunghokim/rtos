#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>
//#include <string.h>

#include <_string.h>

static void memset_func() {
	/* Compare the whole val area and memset arguemnt that is in range of 1 byte(int8_t) */
	char* val;
	for(int i = 1; i < 4000; i++) {
		val = malloc(i * sizeof(char));
		for(int j = -128; j < 128; j++) {
			__memset(val, j, i * sizeof(char));
			for(int x = 0; x < i; x++) {
				assert_int_equal(j, val[x]);
			}
		}

		free(val);
		val = NULL;
	}
}

static void memset_sse_func() {
	char* val;
	for(int i = 1; i < 4000; i++) {
		val = malloc(i * sizeof(char));
		for(int j = -128; j < 128; j++) {
			__memset_sse(val, j, i * sizeof(char));
			for(int x = 0; x < i; x++) {
				//printf("i: %d / j: %d / val[%d]: %d\n", i, j, x, val[x]);
				assert_int_equal(j, val[x]);
			}
		}

		free(val);
		val = NULL;
	}
}

static void memcpy_func() {
	/* Compare the source and the dst memory area */
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 1; j <= i; j++) {
			__memcpy(dst, src, j);		
			
			for(int k = 0; k < j; k++) {
				//printf("memcpy: %d, %d, %d\n", i, j, k);
				assert_int_equal(dst[k], src[k]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memcpy_sse_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);
	
		assert_not_in_range(dst, src, src + i);
		assert_not_in_range(src, dst, dst + i);

		for(int j = 1; j <= i; j++) {
			__memcpy(dst, src, j);		
			for(int k = 0; k < j; k++) {
				assert_int_equal(dst[k], src[k]);
			}
		}	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}	
}

static void memmove_func() {
	char* origin;
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		origin = malloc(i * 2);
		src = origin;
		dst = origin + i;
	
		for(int j = 0; j < i; j++) {
			__memmove(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(origin);
		origin = NULL;
		src = NULL;
		dst = NULL;
	}	
}

static void memmove_sse_func() {
	char* origin;
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		origin = malloc(i * 2);
		src = origin;
		dst = origin + i;
	
		for(int j = 0; j < i; j++) {
			__memmove_sse(dst, src, j);		
			
			for(int x = 0; x < j; x++) {
				assert_int_equal(dst[x], src[x]);
			}
		}	

		free(origin);
		origin = NULL;
		src = NULL;
		dst = NULL;
	}	
}

static void memcmp_func() {
	char* src;
	char* dst;

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);

		memset(src, 0, i);		
		memset(dst, 0, i);		
		
		int rtn = __memcmp(src, dst, i);
		assert_int_equal(0, rtn);	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}

	for(int i = 1; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);

		memset(src, 1, i);
		memset(dst, 2, i);

		int rtn = __memcmp(dst, src, i);
		if(*src > *dst) {
			assert_in_range(rtn, 1, INT32_MAX);		
		} else if(*src < *dst) { 
			assert_in_range(rtn, INT32_MIN, -1);
		} 	
	}
}

static void memcmp_sse_func() {
	char* src;
	char* dst;

	for(int i = 0; i < 4000; i++) {
		src = malloc(i);
		dst = malloc(i);

		memset(src, 0, i);		
		memset(dst, 0, i);		
		
		int rtn = __memcmp_sse(src, dst, i);
		assert_int_equal(0, rtn);	

		free(src);
		src = NULL;
		free(dst);
		dst = NULL;
	}
}

static void bzero_func() {
	char* val;
	for(int i = 0; i < 4000; i++) {
		val = malloc(i);
		__bzero(val, i);						

		for(int j = 0; j < i; j++) {
			assert_int_equal(0, val[j]);
		}
		
		free(val);
		val = NULL;
	}
}	

static void strlen_func() {
	char* str;
	for(int i = 1; i < 4000; i++) {
		str = malloc(i + 1);
		memset(str, 1, i);	
		str[i] = '\0';	
		
		int len = __strlen(str);
		assert_int_equal(i, len);

		free(str);
		str = NULL;
	}
}

static void strstr_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";

	/* Same string at Head */
	char* str = __strstr(text, "`1234567890");		
	assert_memory_equal(str, "`1234567890", strlen("`1234567890"));	
	
	/* Middle */
	str = __strstr(text, "-=qwertyuiop[]asdfghjk");
	assert_memory_equal(str, "-=qwertyuiop[]asdfghjk", strlen("-=qwertyuiop[]asdfghjk"));	

	/* Tail */
	str = __strstr(text, "-=qwertyuiop[]asdfghjk");
	assert_memory_equal(str, "-=qwertyuiop[]asdfghjk", strlen("-=qwertyuiop[]asdfghjk"));	

	/* All */
	str = __strstr(text, "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0");
	assert_memory_equal(str, text, strlen(text));

	/* Not matched */
	str = __strstr(text, "11`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_\0");
	assert_null(str);
}

static void strchr_func() {
	char text[1024] = "`11223344567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	int len = strlen(text);

	for(int i = 0; i < len; i++) {
		if(i < 10 && i != 0 && i % 2 == 0) {
			char* str = __strchr(text, text[i]);				
			assert_memory_equal(str, text + i - 1, strlen(text + i - 1));		
		} else {
			char* str = __strchr(text, text[i]);				
			assert_memory_equal(str, text + i, strlen(text + i));		
		}
	}
}

static void strrchr_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:1ZXCVBNM<>?\0";
	int len = strlen(text);

	for(int i = 0; i < len; i++) {
		char* str;
		if(text[i] == '1') {
			str = __strrchr(text, '1');		
			assert_memory_equal(str, "1ZXCVBNM<>?", strlen("1ZXCVBNM<>?"));
		} else {
			str = __strrchr(text, text[i]);				
			assert_memory_equal(str, text + i, strlen(text + i));		
		}
	}	
}

static void strcmp_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char equal[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in head that is smaller than origin text */
	char not_eqaul_1[1024] = "11234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in middle that is bigger than origin text */
	char not_eqaul_2[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,.!~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in tail that is biggeer than origin text */
	char not_eqaul_3[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNMM?!\0";

	assert_int_equal(0, __strcmp(text, equal));	

	/* When return value is over zero */
	assert_in_range(__strcmp(text, not_eqaul_1), 1, INT32_MAX);
	assert_in_range(__strcmp(text, not_eqaul_2), 1, INT32_MAX);
	/* when retrun value is under zero */	
	assert_in_range(__strcmp(text, not_eqaul_3), INT32_MIN, -1);
	assert_int_not_equal(0, __strcmp(text, not_eqaul_1));	
	assert_int_not_equal(0, __strcmp(text, not_eqaul_2));	
	assert_int_not_equal(0, __strcmp(text, not_eqaul_3));	
}

static void strncmp_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char equal[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in head */
	char not_eqaul_1[1024] = "11234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in middle */
	char not_eqaul_2[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,.!~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	/* Difference in tail */
	char not_eqaul_3[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNMM?!\0";

	/* Comparing from head */
	for(size_t i = 1; i < strlen(text); i++) {
		assert_int_equal(0, __strncmp(text, equal, i));			
	}

	/* Comparing subset from head and forwarding to tail */
	for(size_t i = 1; i < strlen(text); i++) {
		assert_int_equal(0, __strncmp(text + i, equal + i, strlen(text) - i));			
	}

	/* When return value is over zero */
	assert_in_range(__strncmp(text, not_eqaul_1, strlen(text)), 1, INT32_MAX);
	assert_in_range(__strncmp(text, not_eqaul_2, strlen(text)), 1, INT32_MAX);
	/* when retrun value is under zero */	
	assert_in_range(__strncmp(text, not_eqaul_3, strlen(text)), INT32_MIN, -1);

}

static void strdup_func() {
	char text[1024] = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:ZXCVBNM<>?\0";
	char* str;

	for(size_t i = 0; i < strlen(text); i++) {
		str = __strdup(text + i);
		assert_int_equal(0, strncmp(str, text + i, strlen(text) - i));

		free(str);
		str = NULL;
	}
}

static void strtol_func() {
	/* Without integner prefix(e.g. 0x, 0b...) */
	char text[1024] = "12345670qwert\0";
	char text2[1024] = "qwert12345670qwert\0";
	char text3[1024] = "qwert12345670\0";
	char* non_intger;
	long int rtn = 0;
	int base = 0;

		/* base 0 */
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text2 + 8, strlen(text2 + 8));
//
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text3 + 8, strlen(text3 + 8));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

//	base = 8;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text2 + 8, strlen(text2 + 8));
//
//	base = 8;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text3 + 8, strlen(text3 + 8));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

//	base = 10;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text2 + 8, strlen(text2 + 8));
//
//	base = 10;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text3 + 8, strlen(text3 + 8));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

//	base = 16;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text2 + 8, strlen(text2 + 8));
//
//	base = 16;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text3 + 8, strlen(text3 + 8));

	/* With prefix 0(octet)*/
	memset(text, 0, 1024);
	strncpy(text, "012345670qwert", strlen("012345670qwert"));

//	memset(text2, 0, 1024);
//	strncpy(text2, "qwert012345670qwert", strlen("qwert012345670qwert"));
//
//	memset(text, 0, 1024);
//	strncpy(text, "qwert012345670", strlen("qwert012345670"));

		/* base 0 */
	base = 0;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

//	base = 0;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text2 + 9, strlen(text2 + 9));
//
//	base = 0;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text3 + 9, strlen(text3 + 9));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

//	base = 8;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text2 + 9, strlen(text2 + 9));
//
//	base = 8;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 012345670);
//	assert_memory_equal(non_intger, text3 + 9, strlen(text3 + 9));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

//	base = 10;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text2 + 9, strlen(text2 + 9));
//
//	base = 10;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 12345670);
//	assert_memory_equal(non_intger, text3 + 9, strlen(text3 + 9));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

//	base = 16;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text2 + 9, strlen(text2 + 9));
//
//	base = 16;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text3 + 9, strlen(text3 + 9));

	/* With prefix 0x(hex)*/
	memset(text, 0, 1024);
	strncpy(text, "0x12345670qwert", strlen("0x12345670qwert"));

//	memset(text2, 0, 1024);
//	strncpy(text2, "qwert0x12345670qwert", strlen("qwert0x12345670qwert"));
//
//	memset(text3, 0, 1024);
//	strncpy(text3, "qwert0x12345670", strlen("qwert0x12345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));

//	base = 0;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text2 + 10, strlen(text2 + 10));
//
//	base = 0;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text3 + 10, strlen(text3 + 10));

		/* base 8 */
	base = 8;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

//	base = 8;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0);
//	assert_memory_equal(non_intger, text2 + 1, strlen(text2 + 1));
//
//	base = 8;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0);
//	assert_memory_equal(non_intger, text3 + 1, strlen(text3 + 1));

		/* base 10 */
	base = 10;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

//	base = 10;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0);
//	assert_memory_equal(non_intger, text2 + 1, strlen(text2 + 1));
//
//	base = 10;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0);
//	assert_memory_equal(non_intger, text3 + 1, strlen(text3 + 1));

		/* base 16 */
	base = 16;
	rtn = __strtol(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));

//	base = 16;
//	rtn = __strtol(text2, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text2 + 10, strlen(text + 10));
//
//	base = 16;
//	rtn = __strtol(text3, &non_intger, base);
//	assert_int_equal(rtn, 0x12345670);
//	assert_memory_equal(non_intger, text3 + 10, strlen(text + 10));
}

static void strtoll_func() {
	/* Without integner prefix(e.g. 0x, 0b...) */
	char text[1024] = "12345670qwert\0";
	char* non_intger;
	long long int rtn = 0;
	int base = 0;

		/* base 0 */
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 8, strlen(text + 8));



	/* With prefix 0(octet)*/
	memset(text, 0, 1024);
	strncpy(text, "012345670qwert", strlen("012345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 012345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 9, strlen(text + 9));


	/* With prefix 0x(hex)*/
	memset(text, 0, 1024);
	strncpy(text, "0x12345670qwert", strlen("0x12345670qwert"));

		/* base 0 */
	base = 0;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));

		/* base 8 */
	base = 8;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 10 */
	base = 10;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0);
	assert_memory_equal(non_intger, text + 1, strlen(text + 1));

		/* base 16 */
	base = 16;
	rtn = __strtoll(text, &non_intger, base);
	assert_int_equal(rtn, 0x12345670);
	assert_memory_equal(non_intger, text + 10, strlen(text + 10));
}


int main() {
	const struct CMUnitTest UnitTest[] = {
//		cmocka_unit_test(memset_func),
//		cmocka_unit_test(memset_sse_func),
//		cmocka_unit_test(memcpy_func),
//		cmocka_unit_test(memcpy_sse_func),
//		cmocka_unit_test(memcmp_func),
//		cmocka_unit_test(memcmp_sse_func),
//		cmocka_unit_test(memmove_func),
//		cmocka_unit_test(memmove_sse_func),
//		cmocka_unit_test(bzero_func),
		cmocka_unit_test(strlen_func),
		cmocka_unit_test(strstr_func),
		cmocka_unit_test(strchr_func),
		cmocka_unit_test(strrchr_func),
		cmocka_unit_test(strcmp_func),
		cmocka_unit_test(strncmp_func),
		cmocka_unit_test(strdup_func),
		cmocka_unit_test(strtol_func),
		cmocka_unit_test(strtoll_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
