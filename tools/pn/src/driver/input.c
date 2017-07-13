#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "input.h"
#include "../io_mux.h"
#include "../stdio.h"

static bool output_enabled;
static CharInCallback callback;
static int init(void* device, void* data) {
	if(output_enabled)
		return 0; // Already enabled 

	output_enabled = true;
	return 0;
}

static void destroy(int id) {
}

static int _write(int id, const char* buf, int len) {
	if(!output_enabled)
		return -1;

	while(len) {
		len -= write(STDOUT_FILENO, buf, len);
	}

	return 0;
}

static int scroll(int id, int lines) {
	//TODO
	return 0;
}

static void set_cursor(int id, int rows, int cols) {
	//TODO
}

static void get_cursor(int id, int* rows, int* cols) {
	//TODO
}

static void set_render(int id, int is_render) {
	//TODO
}

static int is_render(int id) {
	//TODO
	return 0;
}

DeviceType output_type = DEVICE_TYPE_CHAR_OUT;

CharOut output_driver = {
	.init = init,
	.destroy = destroy,
	.write = _write,
	.scroll = scroll,
	.set_cursor = set_cursor,
	.get_cursor = get_cursor,
	.set_render = set_render,
	.is_render = is_render
};

static char stdin_buffer[4096];
static int slowpath_read_handler(int fd, void* context) {
	int len = read(fd, stdin_buffer, 4096 - 1);
	for(int i = 0; i < len; i++) {
		stdio_putchar(stdin_buffer[i]);
	}

	if(callback) callback();

	return 0;
}

static IOMultiplexer* stdin_io_mux;
static void set_callback(int id, CharInCallback cb) {
	callback = cb;

	stdin_io_mux = calloc(1, sizeof(IOMultiplexer));
	if(!stdin_io_mux) goto error;
	stdin_io_mux->fd = STDIN_FILENO;
	stdin_io_mux->read_handler = slowpath_read_handler;

	if(!io_mux_add(stdin_io_mux, (uint64_t)stdin_io_mux)) goto error;

error:
	if(stdin_io_mux) free(stdin_io_mux);

	return;
}

DeviceType input_type = DEVICE_TYPE_CHAR_IN;

CharIn input_driver = {
	.init = init,
	.destroy = destroy,
	.set_callback = set_callback
};
