#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <util/ring.h>
#include <string.h>
#include <pthread.h>

#define BUFSIZE 4096
#define CHSIZE 26

static char ringbuf[BUFSIZE];
static size_t head = 0;
static size_t tail = 0;

void *thread_write(void *arg);

/**
 * ring_write_func : This function is only checking whether buffer is well writen and read through trailing tail.
 * ring_read_func : This function is overall checking buffer writing and reading, so this function uses pthread to write to buffer continuously.
 */

static void ring_write_func(void **state) {
	int rtn;
	char data = 'a';  //random value
	int comp_tail = 0;
	memset(ringbuf, 0, BUFSIZE);
	head = 0;
	tail = 0;

	for(int i = 0; i < BUFSIZE * 2; i++) {
		rtn = ring_write(ringbuf, head, &tail, BUFSIZE, &data, 1);
		if(rtn == 0)
			ring_read(ringbuf, &head, tail, BUFSIZE, &data, 1);
		else {
			comp_tail = (comp_tail + 1) % BUFSIZE;
			assert_int_equal(tail, comp_tail);
		}
	}
}

static void ring_read_func(void **state) {
	memset(ringbuf, 0, BUFSIZE);
	head = 0;
	tail = 0;
	
	pthread_t write;
	pthread_create(&write, NULL, &thread_write, NULL);
	
	char data;
	char comp_ch;
	int flag = 0;
	int rtn;
	for(int i = 0; i < 10000; i++) {
		rtn = ring_read(ringbuf, &head, tail, BUFSIZE, &data, 1);
		if(rtn == 1) {
			comp_ch = 'a' + flag;
			assert_int_equal(data, comp_ch);	// data checking
			flag++;
			flag %= CHSIZE;
		}
	}
	pthread_join(write, NULL);
}

static void ring_writable_func(void **state) {
	int rtn;
	char data = 'a';  //random value
	int writable = BUFSIZE;
	memset(ringbuf, 0, BUFSIZE);
	head = 0;
	tail = 0;

	assert_int_equal(ring_writable(head, tail, BUFSIZE), writable);

	for(int i = 0; i < BUFSIZE; i++) {
		rtn = ring_write(ringbuf, head, &tail, BUFSIZE, &data, 1);
		if(rtn == 1)
			assert_int_equal(ring_writable(head, tail, BUFSIZE), --writable);
	}
	
	assert_int_equal(ring_writable(head, tail, BUFSIZE), writable);
	for(int i = 0; i < BUFSIZE; i++) {
		rtn = ring_read(ringbuf, &head, tail, BUFSIZE, &data, 1);
		if(rtn == 1)
			assert_int_equal(ring_writable(head, tail, BUFSIZE), ++writable);
	}

}

static void ring_readable_func(void **state) {
	int rtn;
	char data = 'a';  //random value
	int readable = 0;
	memset(ringbuf, 0, BUFSIZE);
	head = 0;
	tail = 0;

	assert_int_equal(ring_readable(head, tail, BUFSIZE), readable);

	for(int i = 0; i < BUFSIZE; i++) {
		rtn = ring_write(ringbuf, head, &tail, BUFSIZE, &data, 1);
		if(rtn == 1)
			assert_int_equal(ring_readable(head, tail, BUFSIZE), ++readable);
	}
	
	assert_int_equal(ring_readable(head, tail, BUFSIZE), readable);
	for(int i = 0; i < BUFSIZE; i++) {
		rtn = ring_read(ringbuf, &head, tail, BUFSIZE, &data, 1);
		if(rtn == 1)
			assert_int_equal(ring_readable(head, tail, BUFSIZE), --readable);
	}
}

void *thread_write(void *arg) {
	char ch = 'a';
	char data[CHSIZE];
	for(int i = 0; i < CHSIZE; i++) {
		data[i] = ch;
		ch++;
	}
	int cnt = 0;
	int rtn;
	for(int i = 0; i < 10000; i++) {
		rtn = ring_write(ringbuf, head, &tail, BUFSIZE, &data[cnt], 1);
		if(rtn == 1)
			cnt = (cnt + 1) % CHSIZE;
	}

	return NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(ring_write_func),	
		cmocka_unit_test(ring_read_func),	
		cmocka_unit_test(ring_readable_func),	
		cmocka_unit_test(ring_writable_func),	
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}

