#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#include "../../../../../tlsf/include/tlsf.h"
#include "../../../../../ext/include/util/ring.h"

char **environ;
extern void* __malloc_pool;
uint64_t __pid;

char* __stdin;
volatile size_t __stdin_head;
volatile size_t __stdin_tail;
volatile size_t __stdin_size;

char* __stdout;
volatile size_t __stdout_head;
volatile size_t __stdout_tail;
volatile size_t __stdout_size;

char* __stderr;
volatile size_t __stderr_head;
volatile size_t __stderr_tail;
volatile size_t __stderr_size;

uint64_t __cpu_frequency;

struct _reent;

void _exit(int code) {
	void(*fn)() = (void*)0;
	fn();
}

int _close_r(struct _reent* r, int file) {
	return -1;
}

 /* pointer to array of char * strings that define the current environment variables */
int _execve_r(struct _reent* r, char *name, char **argv, char **env) {
	return -1;
}

int _fork_r(struct _reent* r) {
	return -1;
}

int _fstat_r(struct _reent* r, int file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}

int _getpid_r(struct _reent* r) {
	return __pid;
}

int getpid() {
	return __pid;
}

int getuid() {
	return 0;
}

int _isatty_r(struct _reent* r, int file) {
	return 1;
}

int _kill_r(struct _reent* r, int pid, int sig) {
	errno = EINVAL;
	return -1;
}

int _link_r(struct _reent* r, char *old, char *new) {
	errno = EMLINK;
	return -1;
}

int _lseek_r(struct _reent* r, int file, int ptr, int dir) {
	return 0;
}

int _mkdir_r(struct _reent* r, const char* pathname, mode_t mode) {
	return -1;
}

int _open_r(struct _reent* r, const char *name, int flags, ...) {
	return -1;
}

int _read_r(struct _reent* r, int file, char *ptr, int len) {
	switch(file) {
		case 0: // stdin
			//while(__stdin_head == __stdin_tail);
			return ring_read(__stdin, &__stdin_head, __stdin_tail, __stdin_size, ptr, len);
		case 1: // stdout
			//while(__stdout_head == __stdout_tail);
			return ring_read(__stdout, &__stdout_head, __stdout_tail, __stdout_size, ptr, len);
		case 2: // stderr
			//while(__stderr_head == __stderr_tail);
			return ring_read(__stderr, &__stderr_head, __stderr_tail, __stderr_size, ptr, len);
		default:
			return -1;
	}
}

int _rename(struct _reent* r, const char* oldpath, const char* newpath) {
	return -1;
}

caddr_t _sbrk_r(struct _reent* r, int incr) {
	extern char _end;
	static char* heap_end;
	char* prev_heap_end;

	if(heap_end == 0) {
		heap_end = &_end;
	}

	prev_heap_end = heap_end;
	heap_end += incr;

	return (caddr_t)prev_heap_end;
}

int _stat_r(struct _reent* r, const char *file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}

inline static uint64_t cpu_tsc() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));

	return time;
}

#define LINUX_CLOCKS_PER_SEC	1000000

clock_t _times_r(struct _reent* r, struct tms *buf) {
	clock_t time = (clock_t)(cpu_tsc() / (__cpu_frequency / LINUX_CLOCKS_PER_SEC));

	if(buf) {
		buf->tms_utime = time;
		buf->tms_stime = 0;
		buf->tms_cutime = 0;
		buf->tms_cstime = 0;
	}

	return time;
}

clock_t times(struct tms *buf) {
	return _times_r(NULL, buf);
}

clock_t clock() {
	return (clock_t)(cpu_tsc() / (__cpu_frequency / LINUX_CLOCKS_PER_SEC));
}

int _unlink_r(struct _reent* r, char *name) {
	errno = ENOENT;
	return -1;
}

int _wait_r(struct _reent* r, int *status) {
	errno = ECHILD;
	return -1;
}

int _write_r(struct _reent* r, int file, char *ptr, int len) {
	switch(file) {
		case 0: // stdin
			return ring_write(__stdin, __stdin_head, &__stdin_tail, __stdin_size, ptr, len);
		case 1: // stdout
			return ring_write(__stdout, __stdout_head, &__stdout_tail, __stdout_size, ptr, len);
		case 2: // stderr
			return ring_write(__stderr, __stderr_head, &__stderr_tail, __stderr_size, ptr, len);
		default:
			return -1;
	}
}

int _gettimeofday_r(struct _reent* r, struct timeval *p, void *z) {
	return 0;
}

extern void* gmalloc(size_t size);

void* _malloc_r(struct _reent* r, size_t size) {
	void* ptr = malloc_ex(size, __malloc_pool);
	if(!ptr)
		ptr = gmalloc(size);

	return ptr;
}

extern void gfree(void* ptr);

void _free_r(struct _reent* r, void* ptr) {
	if(ptr > __malloc_pool && ptr < __malloc_pool + get_total_size(__malloc_pool))
		free_ex(ptr, __malloc_pool);
	else
		gfree(ptr);
}

extern void* grealloc(void* ptr, size_t size);

void* _realloc_r(struct _reent* r, void* ptr, size_t size) {
	if(ptr > __malloc_pool && ptr < __malloc_pool + get_total_size(__malloc_pool))
		return realloc_ex(ptr, size, __malloc_pool);
	else
		return grealloc(ptr, size);
}

extern void* gcalloc(size_t nmemb, size_t size);

void* _calloc_r(struct _reent* r, size_t nmemb, size_t size) {
	void* ptr = calloc_ex(nmemb, size, __malloc_pool);
	if(!ptr)
		return gcalloc(nmemb, size);

}

// GNU C ctype.h imitation
// Below source code is from https://github.com/evanphx/ulysses-libc/, MIT license

#define X(x) (((x)/256 | (x)*256) % 65536)

static const unsigned short __ctype_b_loc_table[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),
	X(0x200),X(0x320),X(0x220),X(0x220),X(0x220),X(0x220),X(0x200),X(0x200),
	X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),
	X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),X(0x200),
	X(0x160),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),
	X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),
	X(0x8d8),X(0x8d8),X(0x8d8),X(0x8d8),X(0x8d8),X(0x8d8),X(0x8d8),X(0x8d8),
	X(0x8d8),X(0x8d8),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),
	X(0x4c0),X(0x8d5),X(0x8d5),X(0x8d5),X(0x8d5),X(0x8d5),X(0x8d5),X(0x8c5),
	X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),
	X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),X(0x8c5),
	X(0x8c5),X(0x8c5),X(0x8c5),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),
	X(0x4c0),X(0x8d6),X(0x8d6),X(0x8d6),X(0x8d6),X(0x8d6),X(0x8d6),X(0x8c6),
	X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),
	X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),X(0x8c6),
	X(0x8c6),X(0x8c6),X(0x8c6),X(0x4c0),X(0x4c0),X(0x4c0),X(0x4c0),X(0x200),
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const unsigned short *const __ctype_b_loc_ptable = __ctype_b_loc_table + 128;

const unsigned short **__ctype_b_loc(void) {
	return (void *)&__ctype_b_loc_ptable;
}

static const int32_t __ctype_tolower_table[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	91,92,93,94,95,96,
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	123,124,125,126,127,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const int32_t *const __ctype_tolower_ptable = __ctype_tolower_table + 128;

const int32_t **__ctype_tolower_loc(void) {
	return (void *)&__ctype_tolower_ptable;
}

static const int32_t __ctype_toupper_table[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	91,92,93,94,95,96,
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	123,124,125,126,127,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const int32_t *const __ctype_toupper_ptable = __ctype_toupper_table + 128;

const int32_t **__ctype_toupper_loc(void) {
	return (void *)&__ctype_toupper_ptable;
}

// Ubuntu's libc compatible functions

#undef stdin
#undef stdout
#undef stderr

struct _IO_FILE* stdin;
struct _IO_FILE* stdout;
struct _IO_FILE* stderr;

int _IO_putc(int ch, struct _IO_FILE* fp) {
	struct _reent *ptr = _REENT;
	_REENT_SMALL_CHECK_INIT (ptr);

	if(fp == stdout) {
		return putc(ch, _stdout_r (ptr));
	}

	if(fp == stderr) {
		return putc(ch, _stderr_r (ptr));
	}

	return -1;
}

int __sprintf_chk(int flag, char *str, const char *format, ...) {
	va_list va;
	va_start(va, format);
	int len = vsprintf(str, format, va);
	va_end(va);

	return len;
}

int __printf_chk (int flag, const char *fmt, ...) {
	int ret;
	va_list ap;
	struct _reent *ptr = _REENT;

	_REENT_SMALL_CHECK_INIT (ptr);
	va_start (ap, fmt);
	ret = _vfprintf_r (ptr, _stdout_r (ptr), fmt, ap);
	va_end (ap);
	return ret;
}

int __fprintf_chk(FILE* fp, int flag, const char* fmt, ...) {
	int ret;
	va_list ap;

	va_start (ap, fmt);
	ret = _vfprintf_r (_REENT, fp, fmt, ap);
	va_end (ap);
	return ret;
}

void* __memcpy_chk(void* dest, const void* src, size_t size, size_t bos) {
	return memcpy(dest, src, size);
}

int __isoc99_scanf(const char *format, ...) {
	int ret;
	va_list ap;
	struct _reent *ptr = _REENT;

	va_start(ap, format);
	ret = _vscanf_r(ptr, format, ap);
	va_end(ap);

	return ret;
}

int __isoc99_sscanf(const char *str, const char *format, ...) {
	int ret;
	va_list ap;
	struct _reent *ptr = _REENT;

	va_start(ap, format);
	ret = _vsscanf_r(ptr, str, format, ap);
	va_end(ap);

	return ret;
}

void __stack_chk_fail() {
	printf("Stack overflow\n");
	while(1) asm("hlt");
}
