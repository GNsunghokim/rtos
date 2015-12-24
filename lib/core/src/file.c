#include <file.h>
#include <string.h>
#include <timer.h>
#include <malloc.h>
#include <util/fifo.h>
#include <util/event.h>
#include <util/map.h>

FIO* __fio;

static Map* request_ids;
static uint64_t event_id = 0;

void file_init() {
	request_ids = map_create(8, NULL, NULL, NULL);
}

static bool poll_event(void* context) {
	if(fifo_size(__fio->output_buffer) > 0) {
		// Peek the first request of fifo
		FIORequest* req = fifo_peek(__fio->output_buffer, 0);

		// Check if the request is for me
		if(map_contains(request_ids, (void*)(uintptr_t)req->id) && __fio->output_lock == 0) {
			__fio->output_lock = 1;

			// If it's mine, now pop
			req = fifo_pop(__fio->output_buffer);

			__fio->output_lock = 0;

			// Callback procedure
			switch(req->type) {
				case FILE_T_OPEN:
				case FILE_T_OPENDIR:;
					void(*open_cb)(int fd, void* context);
					open_cb = map_remove(request_ids, (void*)(uintptr_t)req->id);
					if(open_cb)
						open_cb(req->fd, req->context);
					break;
				case FILE_T_CLOSE:
				case FILE_T_CLOSEDIR:;
					void(*close_cb)(int ret, void* context);
					close_cb = map_remove(request_ids, (void*)(uintptr_t)req->id);
					if(close_cb)
						close_cb(req->fd, req->context);
					break;
				case FILE_T_READ:
				case FILE_T_WRITE:;
					void(*fio_cb)(void* buffer, size_t size, void* context);
					fio_cb = map_remove(request_ids, (void*)(uintptr_t)req->id);
					if(fio_cb)
						fio_cb(req->op.file_io.buffer, req->op.file_io.size, req->context);
					break;
				case FILE_T_READDIR:;
					void(*dio_cb)(Dirent* dir, void* context);
					dio_cb = map_remove(request_ids, (void*)(uintptr_t)req->id);
					if(dio_cb)
						dio_cb(req->op.read_dir.dir, req->context);
					break;
			}

			free(req);
		}

		// If there's no more request in the map, polling ends
		if(map_is_empty(request_ids)) {
			event_id = 0;
			return false;
		}
	}

	return true;
}

static bool push_request(FIORequest* req) {
	if(__fio->input_lock == 0) {
		__fio->input_lock = 1;

		// Check if ring is available
		if(fifo_push(__fio->input_buffer, req)) {
			__fio->input_lock = 0;
			if(event_id == 0)	// We need only one polling event
				event_id = event_busy_add(poll_event, NULL);

			return true;
		}

		__fio->input_lock = 0;
	}

	return false;
}

int file_open(const char* file_name, char* flags, void(*callback)(int fd, void* context), void* context) {
	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;	// TODO: Set errno

	req->type = FILE_T_OPEN;
	req->context = context;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;
	
	memcpy(req->op.open.name, file_name, strlen(file_name));
	memcpy(req->op.open.flags, flags, strlen(flags));

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_close(int fd, void(*callback)(int ret, void* context), void* context) {
	if(fd < 0)
		return -FILE_ERR_BADFD;

	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;

	req->type = FILE_T_CLOSE;
	req->context = context;
	req->fd = fd;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_read(int fd, void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context) {
	if(fd < 0)
		return -FILE_ERR_BADFD;

	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;

	req->type = FILE_T_READ;
	req->context = context;
	req->fd = fd;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;

	req->op.file_io.buffer = buffer;
	req->op.file_io.size = size;

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_write(int fd, const void* buffer, size_t size, void(*callback)(void* buffer, size_t size, void* context), void* context) {
	if(fd < 0)
		return -FILE_ERR_BADFD;

	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;

	req->type = FILE_T_WRITE;
	req->context = context;
	req->fd = fd;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;

	req->op.file_io.buffer = (void*)buffer;
	req->op.file_io.size = size;

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_opendir(const char* dir_name, void(*callback)(int fd, void* context), void* context) {
	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;
	
	req->type = FILE_T_OPENDIR;
	req->context = context;
	memcpy(req->op.open.name, dir_name, strlen(dir_name));
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_readdir(int fd, Dirent* dir, void(*callback)(Dirent* dir, void* context), void* context) {
	if(fd < 0)
		return -FILE_ERR_BADFD;

	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;

	req->type = FILE_T_READDIR;
	req->context = context;
	req->fd = fd;
	req->op.read_dir.dir = dir;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;

	map_put(request_ids, (void*)(uintptr_t)req->id, callback);

	if(!push_request(req))
		return -2;

	return 0;
}

int file_closedir(int fd, void(*callback)(int ret, void* context), void* context) {
	if(fd < 0)
		return -FILE_ERR_BADFD;

	FIORequest* req = malloc(sizeof(FIORequest));
	if(!req)
		return -FILE_ERR_NOSPC;

	req->type = FILE_T_CLOSEDIR;
	req->context = context;
	req->fd = fd;
	req->id = (__fio->request_id++) % FIO_MAX_REQUEST_ID;
	
	map_put(request_ids, (void*)(uintptr_t)req->id, callback);
	
	if(!push_request(req))
		return -2;

	return 0;
}
