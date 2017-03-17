//#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>

#include <xmmintrin.h>
#include <smmintrin.h>

void *__memset(void *s, int c, size_t n) {
	uint64_t c8;
	int8_t *p = (void*)&c8;

	for(int i = 0; i < 8; i++)
		p[i] = (int8_t)c;

	uint64_t* d = s;

	int count = n / 8;
	while(count--)
		*d++ = c8;

	uint8_t* d2 = (uint8_t*)d;
	count = n % 8;
	while(count--)
		*d2++ = c;

	return s;
}

void *__memset_sse(void *dst, int value, size_t len) {
	uint8_t* a = dst;

	int aligned_a = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1));

	/* aligned */
	if(aligned_a) {
		while(len && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
			a[i] = (uint8_t)value;

			i++;
			len--;
		}
	}

	if(len >= 16) {
		uint32_t buf_32 = value;
		buf_32 |= (buf_32 << 8);
		buf_32 |= (buf_32 << 16);

		if(len >= 16) {
			__m128i r1 = _mm_set_epi32(buf_32, buf_32, buf_32, buf_32);

			while(len >= 128) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);
				_mm_store_si128((__m128i*)&a[i + 32], r1);
				_mm_store_si128((__m128i*)&a[i + 48], r1);
				_mm_store_si128((__m128i*)&a[i + 64], r1);
				_mm_store_si128((__m128i*)&a[i + 80], r1);
				_mm_store_si128((__m128i*)&a[i + 96], r1);
				_mm_store_si128((__m128i*)&a[i + 112], r1);

				i += 128;
				len -= 128;
			}

			if(len >= 64) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);
				_mm_store_si128((__m128i*)&a[i + 32], r1);
				_mm_store_si128((__m128i*)&a[i + 48], r1);

				i += 64;
				len -= 64;
			}

			if(len >= 32) {
				_mm_store_si128((__m128i*)&a[i], r1);
				_mm_store_si128((__m128i*)&a[i + 16], r1);

				i += 32;
				len -= 32;
			}

			if(len >= 16) {
				_mm_store_si128((__m128i*)&a[i], r1);

				i += 16;
				len -= 16;
			}
		}

		while(len >= 4) {
			*(uint32_t*)(&a[i]) = buf_32;

			i += 4;
			len -= 4;
		}
	}

	while(len) {
		a[i] = (uint8_t)value;

		i++;
		len--;
	}

	return dst;
}

void* __memcpy_sse(void *dst, const void *src, size_t len) {
	uint8_t* a = (uint8_t*)dst;
	uint8_t* b = (uint8_t*)src;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1));
	aligned_b = ((uintptr_t)b & (sizeof(__m128i)-1));

	/* Not aligned */
	if(aligned_a != aligned_b) {
		while(len) {
			a[i] = b[i];

			i++;
			len--;
		}

		return dst;
	}

	/* aligned */
	if(aligned_a) {
		while(len && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
			a[i] = b[i];

			i++;
			len--;
		}
	}

	while(len >= 128) {
		__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
		__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
		__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
		__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
		__m128i r5 = _mm_loadu_si128((__m128i*)&(b[i + 64])); //16byte
		__m128i r6 = _mm_loadu_si128((__m128i*)&(b[i + 80])); //16byte
		__m128i r7 = _mm_loadu_si128((__m128i*)&(b[i + 96])); //16byte
		__m128i r8 = _mm_loadu_si128((__m128i*)&(b[i + 112])); //16byte
		_mm_store_si128((__m128i*)&a[i], r1);
		_mm_store_si128((__m128i*)&a[i + 16], r2);
		_mm_store_si128((__m128i*)&a[i + 32], r3);
		_mm_store_si128((__m128i*)&a[i + 48], r4);
		_mm_store_si128((__m128i*)&a[i + 64], r5);
		_mm_store_si128((__m128i*)&a[i + 80], r6);
		_mm_store_si128((__m128i*)&a[i + 96], r7);
		_mm_store_si128((__m128i*)&a[i + 112], r8);

		i += 128;
		len -= 128;
	}

	if(len >= 64) {
		__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
		__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
		__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
		__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
		_mm_store_si128((__m128i*)&a[i], r1);
		_mm_store_si128((__m128i*)&a[i + 16], r2);
		_mm_store_si128((__m128i*)&a[i + 32], r3);
		_mm_store_si128((__m128i*)&a[i + 48], r4);

		i += 64;
		len -= 64;
	}

	if(len >= 32) {
		__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
		__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
		_mm_store_si128((__m128i*)&a[i], r1);
		_mm_store_si128((__m128i*)&a[i + 16], r2);

		i += 32;
		len -= 32;
	}

	if(len >= 16) {
		__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
		_mm_store_si128((__m128i*)&a[i], r1);

		i += 16;
		len -= 16;
	}

	while(len >= 4) {
		*(uint32_t*)(&a[i]) = *(uint32_t*)(&b[i]);

		i += 4;
		len -= 4;
	}

	while(len) {
		a[i] = b[i];

		i++;
		len--;
	}

	return dst;
}

void * __attribute__ (( noinline )) __memcpy ( void *dest, const void *src,
					       size_t len ) {
	void *edi = dest;
	const void *esi = src;
	int discard_ecx;

	/* We often do large dword-aligned and dword-length block
	 * moves.  Using movsl rather than movsb speeds these up by
	 * around 32%.
	 */
	__asm__ __volatile__ ( "rep movsl"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len >> 2 )
			       : "memory" );
	__asm__ __volatile__ ( "rep movsb"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len & 3 )
			       : "memory" );
	return dest;
}

void* __memmove_sse(void *dst, const void *src, size_t len) {
	uint8_t* a = (uint8_t*)dst;
	uint8_t* b = (uint8_t*)src;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	if(src < dst && dst < src + len) {
		/* Destructive overlap...have to copy backwards */
		i = len;
		aligned_a = ((uintptr_t)&a[i] & (sizeof(__m128i)-1));
		aligned_b = ((uintptr_t)&b[i] & (sizeof(__m128i)-1));

		/* Not aligned */
		if(aligned_a != aligned_b) {
			while(i) {
				i--;
				a[i] = b[i];
			}

			return dst;
		}

		/* align */
		if(aligned_a) {
			while(i && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
				i--;
				a[i] = b[i];
			}
		}

		  while(i >= 128) {
			i -= 128;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			__m128i r5 = _mm_loadu_si128((__m128i*)&(b[i + 64])); //16byte
			__m128i r6 = _mm_loadu_si128((__m128i*)&(b[i + 80])); //16byte
			__m128i r7 = _mm_loadu_si128((__m128i*)&(b[i + 96])); //16byte
			__m128i r8 = _mm_loadu_si128((__m128i*)&(b[i + 112])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);
			_mm_store_si128((__m128i*)&a[i + 64], r5);
			_mm_store_si128((__m128i*)&a[i + 80], r6);
			_mm_store_si128((__m128i*)&a[i + 96], r7);
			_mm_store_si128((__m128i*)&a[i + 112], r8);
		}

		if(i >= 64) {
			i -= 64;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);

		}

		if(i >= 32) {
			i -= 32;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);

		}

		if(i >= 16) {
			i -= 16;
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
		}

		while(i) {
			--i;
			a[i] = b[i];
		}
	} else {
		aligned_a = ((uintptr_t)a & (sizeof(__m128i) - 1));
		aligned_b = ((uintptr_t)b & (sizeof(__m128i) - 1));

		/* Not aligned */
		if(aligned_a != aligned_b) {
			while(len) {
				a[i] = b[i];

				i++;
				len--;
			}

			return dst;
		}

		/* aligned */
		if(aligned_a) {
			while(len && ((uintptr_t) &a[i] & (sizeof(__m128i)-1))) {
				a[i] = b[i];

				i++;
				len--;
			}
		}

		  while(len >= 128) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			__m128i r5 = _mm_loadu_si128((__m128i*)&(b[i + 64])); //16byte
			__m128i r6 = _mm_loadu_si128((__m128i*)&(b[i + 80])); //16byte
			__m128i r7 = _mm_loadu_si128((__m128i*)&(b[i + 96])); //16byte
			__m128i r8 = _mm_loadu_si128((__m128i*)&(b[i + 112])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);
			_mm_store_si128((__m128i*)&a[i + 64], r5);
			_mm_store_si128((__m128i*)&a[i + 80], r6);
			_mm_store_si128((__m128i*)&a[i + 96], r7);
			_mm_store_si128((__m128i*)&a[i + 112], r8);

			i += 128;
			len -= 128;
		}

		if(len >= 64) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			__m128i r3 = _mm_loadu_si128((__m128i*)&(b[i + 32])); //16byte
			__m128i r4 = _mm_loadu_si128((__m128i*)&(b[i + 48])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);
			_mm_store_si128((__m128i*)&a[i + 32], r3);
			_mm_store_si128((__m128i*)&a[i + 48], r4);

			i += 64;
			len -= 64;
		}

		if(len >= 32) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			__m128i r2 = _mm_loadu_si128((__m128i*)&(b[i + 16])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);
			_mm_store_si128((__m128i*)&a[i + 16], r2);

			i += 32;
			len -= 32;
		}

		if(len >= 16) {
			__m128i r1 = _mm_loadu_si128((__m128i*)&(b[i])); //16byte
			_mm_store_si128((__m128i*)&a[i], r1);

			i += 16;
			len -= 16;
		}

		while(len >= 4) {
			*(uint32_t*)(&a[i]) = *(uint32_t*)(&b[i]);

			i += 4;
			len -= 4;
		}

		while(len) {
			a[i] = b[i];

			i++;
			len--;
		}
	}

	return dst;
}
/**
 * Copy memory area backwards
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * __attribute__ (( noinline )) __memcpy_reverse ( void *dest,
						       const void *src,
						       size_t len ) {
	void *edi = ( dest + len - 1 );
	const void *esi = ( src + len - 1 );
	int discard_ecx;

	/* Assume memmove() is not performance-critical, and perform a
	 * bytewise copy for simplicity.
	 */
	__asm__ __volatile__ ( "std\n\t"
			       "rep movsb\n\t"
			       "cld\n\t"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ),
				 "2" ( len )
			       : "memory" );
	return dest;
}

void * __memmove( void *dest, const void *src, size_t len ) {

	if ( dest <= src ) {
		return __memcpy ( dest, src, len );
	} else {
		return __memcpy_reverse ( dest, src, len );
	}
}

void* __memmove_chk(void* dest, const void* src, size_t size, size_t bos) {
	return __memmove(dest, src, size);
}

int __memcmp_sse(const void *dst, const void *src, size_t len) {
	uint8_t* a = (uint8_t*)dst;
	uint8_t* b = (uint8_t*)src;

	if(!len)
		return 0;

	int aligned_a = 0, aligned_b = 0;
	int i = 0;

	aligned_a = ((uintptr_t)a & (sizeof(__m128i)-1));
	aligned_b = ((uintptr_t)b & (sizeof(__m128i)-1));

	while(len) {
		if (a[i] != b[i])
			return b[i] - a[i];

		i++;
		len--;
	}

	return 0;
	/* Not aligned */
	if(aligned_a != aligned_b) {
		while(len) {
			if (a[i] != b[i])
				return b[i] - a[i];

			i++;
			len--;
		}

		return 0;
	}

	/* aligned */
	if(aligned_a) {
		while(len && (((uintptr_t) &a[i]) & (sizeof(__m128i)-1))) {
			if(a[i] != b[i]) {
				return b[i] - a[i];
			}

			i++;
			len--;
		}
	}

	while(len >= 16) {
		__m128i x = _mm_loadu_si128((__m128i*)&(a[i])); //16byte
		__m128i y = _mm_loadu_si128((__m128i*)&(b[i])); //16byte

		__m128i cmp = _mm_cmpeq_epi8(x, y);

		uint16_t result = (uint16_t)_mm_movemask_epi8(cmp);
		if(result != 0xffffU) {
			result = ~result;

			while(!(result & 0x1)) {
				result = result >> 1;
				i++;
			}

			return b[i] - a[i];
		}

		i += 16;
		len -= 16;
	}

	while(len >= 4) {
		if(*(uint32_t*)(&a[i]) != *(uint32_t*)(&b[i])) {
			break;
		}

		i += 4;
		len -= 4;
	}

	while(len) {
		if(a[i] != b[i])
			return b[i] - a[i];

		i++;
		len--;
	}

	return 0;
}

int __memcmp(const void* v1, const void* v2, size_t size) {
	const uint64_t* d = v1;
	const uint64_t* s = v2;

	//unsigned int count = size / 8;
	while(size > 8) {
		if(*d != *s) {
			break;
		}

		s++;
		d++;
		size -= 8;
	}

	const uint8_t* d2 = (uint8_t*)d;
	const uint8_t* s2 = (uint8_t*)s;
	//count = size % 8;
	while(size) {
		if(*d2 != *s2)
			return *s2 - *d2;

		s2++;
		d2++;
		size--;
	}

	return 0;
}

void __bzero(void* dest, size_t size) {
	uint64_t* d = dest;

	int count = size / 8;
	while(count--)
		*d++ = 0;

	uint8_t* d2 = (uint8_t*)d;
	count = size % 8;
	while(count--)
		*d2++ = 0;
}

size_t __strlen(const char* s) {
	int i = 0;
	while(s[i] != '\0')
		i++;

	return i;
}

char* __strstr(const char* haystack, const char* needle) {
	const char* hs = haystack;
	while(*hs != 0) {
		const char* s = hs;
		const char* d = needle;
		while(*d != '\0') {
			if(*s != *d)
				break;

			s++;
			d++;
		}

		if(*d == '\0')
			return (char*)hs;

		hs++;
	}

	return NULL;
}

char* __strchr(const char* s, int c) {
	char* ch = (char*)s;

	while(*ch != c && *ch != '\0')
		ch++;

	return *ch == c ? ch : NULL;
}

char* __strrchr(const char* s, int c) {
	char* ch = (char*)s;

	while(*ch)
		ch++;

	ch--;

	while(*ch != c && *ch != '\0')
		ch--;

	return *ch == c ? ch : NULL;
}

int __strcmp(const char* s, const char* d) {
	while(*s != '\0' && *d != '\0') {
		if(*s != *d)
			return *s - *d;

		s++;
		d++;
	}

	return *s - *d;
}

int __strncmp(const char* s, const char* d, size_t size) {
	if(size == 0)
		return 0;

	while(*s != '\0' && *d != '\0') {
		if(*s != *d)
			return *s - *d;

		s++;
		d++;

		if(--size == 0)
			return 0;
	}

	return *s - *d;
}

char* __strdup(const char* source) {
	int str_len =  __strlen(source) + 1;
	char* dest = malloc(str_len);
	__memcpy(dest, source, str_len);

	return dest;
}

long int __strtol(const char *nptr, char **endptr, int base) {
	if(base == 0) {
		if(nptr[0] == '0') {
			if((nptr[1] == 'x' || nptr[1] == 'X')) {
				base = 16;
				nptr += 2;
			} else {
				base = 8;
				nptr += 1;
			}
		} else {
			base = 10;
		}
	}

	long int value = 0;

	char* p = (char*)nptr;
	while(1) {
		switch(base) {
			case 16:
				if(*p >= 'a' && *p <= 'f') {
					value = value * base + *(p++) - 'a' + 10;
					continue;
				} else if(*p >= 'A' && *p <= 'F') {
					value = value * base + *(p++) - 'A' + 10;
					continue;
				}
			case 10:
				if(*p >= '0' && *p <= '9') {
					value = value * base + *(p++) - '0';
					continue;
				}
			case 8:
				if(*p >= '0' && *p <= '7') {
					value = value * base + *(p++) - '0';
					continue;
				}
		}

		if(endptr != NULL)
			*endptr = p;

		return value;
	}
}

long long int __strtoll(const char *nptr, char **endptr, int base) {
	if(base == 0) {
		if(nptr[0] == '0') {
			if((nptr[1] == 'x' || nptr[1] == 'X')) {
				base = 16;
				nptr += 2;
			} else {
				base = 8;
				nptr += 1;
			}
		} else {
			base = 10;
		}
	}

	long long int value = 0;

	char* p = (char*)nptr;
	while(1) {
		switch(base) {
			case 16:
				if(*p >= 'a' && *p <= 'f') {
					value = value * base + *(p++) - 'a' + 10;
					continue;
				} else if(*p >= 'A' && *p <= 'F') {
					value = value * base + *(p++) - 'A' + 10;
					continue;
				}
			case 10:
				if(*p >= '0' && *p <= '9') {
					value = value * base + *(p++) - '0';
					continue;
				}
			case 8:
				if(*p >= '0' && *p <= '7') {
					value = value * base + *(p++) - '0';
					continue;
				}
		}

		if(endptr != NULL)
			*endptr = p;

		return value;
	}
}
