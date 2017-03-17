#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <file.h>

#include <util/fifo.h>
#include <util/map.h>
#include <tlsf.h>
#include <fio.h>
#include <timer.h>
#include <util/event.h>
#include <_malloc.h>

#include <malloc.h>
#include <pthread.h>
#include <string.h>

extern FIO* __fio;
extern void* __malloc_pool;

static void file_init_func() {
	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);

	file_init();

	/* Can't check that request_ids has been created */
	assert_int_equal(0, __fio->event_id);
}

void open_callback(int fd, void* context) {
	int* _context = (int*)context;
	assert_int_equal(fd, *_context);
}

static void file_open_func() {
	int file_fd = 0;
	int file_context = 0;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);					
	
	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();

	/* check context & fd unitl get to Max request id */ 
	for(int i = 0; i < 256; i++) {
		file_context = i;
		int status = file_open("Test", "r", open_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;

		/* FIORequest that put in to output_buffer of __fio */
		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_OPEN;
		output_buffer->context = &file_context;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;
		output_buffer->fd = file_fd = i;

		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(FIO_OK, status);

		event_loop();
	}

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void close_callback(int ret, void* context) {
	int* _context = (int*)context;
	assert_int_equal(ret, *_context);
}

static void file_close_func() {
	int file_fd = 0;
	int file_context = 0;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);

	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();
	
	for(int i = 0; i < 256; i++) {
		file_context = file_fd = i;

		int status = file_close(file_fd, close_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;

		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_CLOSE;
		output_buffer->context = &file_context;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;
		output_buffer->fd = file_fd;

		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(status, FIO_OK);

		event_loop();
	}	

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void read_callback(void* buffer, int size, void* context) {
	char _buffer[1024];
	memset(_buffer, 0, 1024);
	sprintf(_buffer, "Read Test %d", *(int*)context);
	assert_memory_equal(_buffer, buffer, strlen(buffer));
	assert_int_equal(1024, size);	
}

static void file_read_func() {
	int file_fd = 0;
	int file_context = 0;

	int size = 1024;
	char* buffer;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	buffer = malloc(size); 

	__fio = fio_create(__malloc_pool);

	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();

	for(int i = 0; i < 256; i++) {
		file_context = file_fd = i;
		
		sprintf(buffer, "Read Test %d", file_fd);
		int status = file_read(file_fd, buffer, size, read_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;
		
		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_READ;
		output_buffer->context = &file_context;
		output_buffer->fd = file_fd;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;

		output_buffer->op.file_io.buffer = buffer;
		output_buffer->op.file_io.size = size;
		
		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(status, FIO_OK);

		event_loop();
	}

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void write_callback(void* buffer, int size, void* context) {
	char _buffer[1024];
	memset(_buffer, 0, 1024);
	sprintf(_buffer, "Write Test %d", *(int*)context);
	assert_memory_equal(_buffer, buffer, strlen(buffer));
	assert_int_equal(1024, size);	
}

static void file_write_func() {
	int file_fd = 0;
	int file_context = 0;

	int size = 1024;
	char* buffer;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	buffer = malloc(size); 

	__fio = fio_create(__malloc_pool);

	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();

	for(int i = 0; i < 256; i++) {
		file_context = file_fd = i;
		
		sprintf(buffer, "Write Test %d", file_fd);
		int status = file_read(file_fd, buffer, size, write_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;

		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_WRITE;
		output_buffer->context = &file_context;
		output_buffer->fd = file_fd;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;

		output_buffer->op.file_io.buffer = buffer;
		output_buffer->op.file_io.size = size;
		
		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(status, FIO_OK);

		event_loop();
	}

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void opendir_callback(int fd, void* context) {
	int* _context = (int*)context;
	assert_int_equal(fd, *_context);
}

static void file_opendir_func() {
	int file_fd = 0;
	int file_context = 0;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);					
	
	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();

	/* check context & fd unitl get to Max request id */ 
	for(int i = 0; i < 256; i++) {
		file_context = i;
		int status = file_opendir("Test", opendir_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;

		/* FIORequest that put in to output_buffer of __fio */
		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_OPENDIR;
		output_buffer->context = &file_context;

		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;
		output_buffer->fd = file_fd = i;

		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(FIO_OK, status);

		event_loop();
	}

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void readdir_callback(Dirent* dir, void* context) {
	char _buffer[1024];
	memset(_buffer, 0, 1024);
	sprintf(_buffer, "Read Test %d", *(int*)context);
	assert_memory_equal(_buffer, dir->name, strlen(_buffer));
}

static void file_readdir_func() {
	int file_fd = 0;
	int file_context = 0;

	int size = 1024;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);

	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();

	Dirent* buffer = malloc(sizeof(Dirent));

	for(int i = 0; i < 256; i++) {
		file_context = file_fd = i;
		
		sprintf(buffer->name, "Read Test %d", file_fd);
		int status = file_readdir(file_fd, buffer, readdir_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;
		
		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_READDIR;
		output_buffer->context = &file_context;
		output_buffer->fd = file_fd;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;

		output_buffer->op.file_io.buffer = buffer;
		output_buffer->op.file_io.size = size;
		
		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(status, FIO_OK);

		event_loop();
	}

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

void closedir_callback(int ret, void* context) {
	assert_int_equal(ret, *(int*)context);
}

static void file_closedir_func() {
	int file_fd = 0;
	int file_context = 0;

	__malloc_pool = malloc(0x40000);
	init_memory_pool(0x40000, __malloc_pool, 0);

	__fio = fio_create(__malloc_pool);

	timer_init("Intel(R) Core(TM) i5-4670 CPU @ 3.40GHz");
	event_init();
	file_init();
	
	for(int i = 0; i < 256; i++) {
		file_context = file_fd = i;

		int status = file_closedir(file_fd, closedir_callback, &file_context);

		__fio->output_buffer->head = i;
		__fio->output_buffer->tail = i + 1;

		FIORequest* output_buffer = malloc(sizeof(FIORequest));
		output_buffer->type = FILE_T_CLOSEDIR;
		output_buffer->context = &file_context;
		output_buffer->id = (__fio->request_id - 1) % FIO_MAX_REQUEST_ID;
		output_buffer->fd = file_fd;

		__fio->output_buffer->array[i] = output_buffer;

		assert_int_equal(status, FIO_OK);

		event_loop();
	}	

	fifo_destroy(__fio->input_buffer);
	fifo_destroy(__fio->output_buffer);
	__free(__fio, __malloc_pool);
	
	destroy_memory_pool(__malloc_pool);	
	free(__malloc_pool);
	__malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(file_init_func),
		cmocka_unit_test(file_open_func),
		cmocka_unit_test(file_close_func),
		cmocka_unit_test(file_read_func),
		cmocka_unit_test(file_write_func),
		cmocka_unit_test(file_opendir_func),
		cmocka_unit_test(file_readdir_func),
		cmocka_unit_test(file_closedir_func)
	};

	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
