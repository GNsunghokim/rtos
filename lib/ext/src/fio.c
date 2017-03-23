#include <stdio.h>
#include <util/fifo.h>
#include <_malloc.h>
#include <fio.h>

FIO* fio_create(void* pool) {
	FIO* fio = __malloc(sizeof(FIO), pool);
	if(!fio) {
		printf("FIO malloc error\n");
		return NULL;
	}

	fio->input_buffer = fifo_create(FIO_INPUT_BUFFER_SIZE, pool);
	if(!fio->input_buffer) {
		printf("fifo creation error\n");
		__free(fio, pool);
		return NULL;
	}

	fio->output_buffer = fifo_create(FIO_OUTPUT_BUFFER_SIZE, pool);
	if(!fio->output_buffer) {
		printf("fifo creation error\n");
		fifo_destroy(fio->input_buffer);
		__free(fio, pool);
		return NULL;
	}

	return fio;
}
