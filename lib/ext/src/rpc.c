#include <string.h>
#include <strings.h>
#include <malloc.h>
#include "control/rpc.h"

struct {
	uint32_t 	id;
	bool(*callback)(RPC* rpc, void* context);
	void*		context;
} RPC_Callback;

#define MAKE_WRITE(TYPE)					\
static int write_##TYPE(RPC* rpc, TYPE##_t v) {			\
	int len = sizeof(TYPE##_t);				\
	if(rpc->wbuf_index + len > RPC_BUFFER_SIZE)		\
		return 0;					\
								\
	memcpy(rpc->wbuf + rpc->wbuf_index, &v, len);		\
	rpc->wbuf_index += len;					\
								\
	return len;						\
}

#define MAKE_READ(TYPE)						\
static int read_##TYPE(RPC* rpc, TYPE##_t* v) {			\
	int len = sizeof(TYPE##_t);				\
	if(rpc->rbuf_read + len > rpc->rbuf_index) {		\
		int len2 = rpc->read(rpc, 			\
			rpc->rbuf + rpc->rbuf_index, 		\
			RPC_BUFFER_SIZE - rpc->rbuf_index);	\
		if(len2 < 0) {					\
			return len2;				\
		}						\
								\
		rpc->rbuf_index += len2;			\
	}							\
								\
	if(rpc->rbuf_read + len > rpc->rbuf_index)		\
		return 0;					\
								\
	memcpy(v, rpc->rbuf + rpc->rbuf_read, len);		\
	rpc->rbuf_read += len;					\
								\
	return len;						\
}

MAKE_READ(uint8)
MAKE_WRITE(uint8)
MAKE_READ(uint16)
MAKE_WRITE(uint16)
MAKE_READ(uint32)
MAKE_WRITE(uint32)
MAKE_READ(uint64)
MAKE_WRITE(uint64)

MAKE_READ(int32)
MAKE_WRITE(int32)

#define read_bool(RPC, DATA)	read_uint8((RPC), (uint8_t*)(DATA))
#define write_bool(RPC, DATA)	write_uint8((RPC), (uint8_t)(DATA))

static int write_string(RPC* rpc, const char* v) {
	uint16_t len0;
	if(!v)
		len0 = 0;
	else
		len0 = strlen(v);

	uint16_t len = sizeof(uint16_t) + len0;
	if(rpc->wbuf_index + len > RPC_BUFFER_SIZE)
		return 0;

	memcpy(rpc->wbuf + rpc->wbuf_index, &len0, sizeof(uint16_t));
	memcpy(rpc->wbuf + rpc->wbuf_index + sizeof(uint16_t), v, len0);
	rpc->wbuf_index += len;

	return len;
}

static int read_string(RPC* rpc, char** v, uint16_t* len) {
	*len = 0;

	uint16_t len0;
	memcpy(&len0, rpc->rbuf + rpc->rbuf_read, sizeof(uint16_t));
	uint16_t len1 = sizeof(uint16_t) + len0;

	if(rpc->rbuf_read + len1 > rpc->rbuf_index) {
		int len2 = rpc->read(rpc, rpc->rbuf + rpc->rbuf_index,
			RPC_BUFFER_SIZE - rpc->rbuf_index);
		if(len2 < 0) {
			return len2;
		}

		rpc->rbuf_index += len2;
	}

	if(rpc->rbuf_read + len1 > rpc->rbuf_index)
		return 0;

	*len = len0;
	if(v != NULL)
		*v = (char*)(rpc->rbuf + rpc->rbuf_read + sizeof(uint16_t));
	rpc->rbuf_read += len1;

	return len1;
}

static int write_bytes(RPC* rpc, void* v, int32_t size) {
	int32_t len = sizeof(int32_t) + (size < 0 ? 0 : size);
	if(rpc->wbuf_index + len > RPC_BUFFER_SIZE)
		return 0;

	memcpy(rpc->wbuf + rpc->wbuf_index, &size, sizeof(int32_t));
	if(size > 0)
		memcpy(rpc->wbuf + rpc->wbuf_index + sizeof(int32_t), v, size);
	rpc->wbuf_index += len;

	return len;
}

static int read_bytes(RPC* rpc, void** v, int32_t* len) {
	*len = 0;

	int32_t len0;
	memcpy(&len0, rpc->rbuf + rpc->rbuf_read, sizeof(int32_t));
	int32_t len1 = sizeof(int32_t) + (len0 < 0 ? 0 : len0);

	if(rpc->rbuf_read + len1 > rpc->rbuf_index) {
		int len2 = rpc->read(rpc, rpc->rbuf + rpc->rbuf_index,
			RPC_BUFFER_SIZE - rpc->rbuf_index);
		if(len2 < 0) {
			return len2;
		}

		rpc->rbuf_index += len2;
	}

	if(rpc->rbuf_read + len1 > rpc->rbuf_index)
		return 0;

	*len = len0;
	if(v != NULL) {
		if(len0 > 0) {
			*v = (void*)(rpc->rbuf + rpc->rbuf_read + sizeof(int32_t));
		} else {
			*v = NULL;
		}
	}
	rpc->rbuf_read += len1;

	return len1;
}

#define INIT()								\
	__attribute__((__unused__)) int _len = 0;			\
	__attribute__((__unused__)) int _size = 0;			\
	__attribute__((__unused__)) int _rbuf_read = rpc->rbuf_read;	\
	__attribute__((__unused__)) int _wbuf_index = rpc->wbuf_index;

#define ROLLBACK()			\
	rpc->rbuf_read = _rbuf_read;	\
	rpc->wbuf_index = _wbuf_index;

#define READ(VALUE)			\
if((_len = (VALUE)) <= 0) {		\
	ROLLBACK();			\
	return _len; 			\
} else {				\
	_size += _len;			\
}

#define READ2(VALUE, FAILED)		\
if((_len = (VALUE)) <= 0) {		\
	ROLLBACK();			\
	FAILED();			\
	return _len; 			\
} else {				\
	_size += _len;			\
}

#define WRITE(VALUE)			\
if((_len = (VALUE)) <= 0) {		\
	ROLLBACK();			\
	return _len; 			\
} else {				\
	_size += _len;			\
}

#define RETURN()	return _size;

#define INIT2()					\
	int _wbuf_index = rpc->wbuf_index;

#define WRITE2(VALUE)			\
if((VALUE) <= 0) {			\
	rpc->wbuf_index = _wbuf_index;	\
	return;				\
}

#define RETURN2()			\
if(rpc->wbuf_index > 0 && rpc->write && wbuf_flush(rpc) < 0 && rpc->close) {	\
	rpc->close(rpc);							\
	return;									\
}

static int write_vm(RPC* rpc, VMSpec* vm) {
	INIT();

	WRITE(write_bool(rpc, vm != NULL));
	if(vm == NULL)
		RETURN();

	WRITE(write_uint32(rpc, vm->id));
	WRITE(write_uint32(rpc, vm->core_size));
	WRITE(write_uint32(rpc, vm->memory_size));
	WRITE(write_uint32(rpc, vm->storage_size));

	WRITE(write_uint16(rpc, vm->nic_count));
	for(int i = 0; i < vm->nic_count; i++) {
		WRITE(write_uint64(rpc, vm->nics[i].mac));
		//WRITE(write_uint32(rpc, vm->nics[i].port));
		WRITE(write_string(rpc, vm->nics[i].dev));
		WRITE(write_uint32(rpc, vm->nics[i].input_buffer_size));
		WRITE(write_uint32(rpc, vm->nics[i].output_buffer_size));
		WRITE(write_uint64(rpc, vm->nics[i].input_bandwidth));
		WRITE(write_uint64(rpc, vm->nics[i].output_bandwidth));
		WRITE(write_uint8(rpc, vm->nics[i].padding_head));
		WRITE(write_uint8(rpc, vm->nics[i].padding_tail));
		WRITE(write_uint32(rpc, vm->nics[i].pool_size));
	}

	WRITE(write_uint16(rpc, vm->argc));
	for(int i = 0; i < vm->argc; i++) {
		WRITE(write_string(rpc, vm->argv[i]));
	}

	RETURN();
}

static void vm_free(VMSpec* vm);

static int read_vm(RPC* rpc, VMSpec** vm2) {
	INIT();

	bool is_not_null;
	READ(read_bool(rpc, &is_not_null));
	if(!is_not_null) {
		*vm2 = NULL;
		RETURN();
	}

	VMSpec* vm = malloc(sizeof(VMSpec));
	if(!vm) {
		return -10;
	}
	bzero(vm, sizeof(VMSpec));

	void failed() {
		if(vm)
			vm_free(vm);
	}

	READ2(read_uint32(rpc, &vm->id), failed);
	READ2(read_uint32(rpc, &vm->core_size), failed);
	READ2(read_uint32(rpc, &vm->memory_size), failed);
	READ2(read_uint32(rpc, &vm->storage_size), failed);
	READ2(read_uint16(rpc, &vm->nic_count), failed);

	vm->nics = malloc(vm->nic_count * sizeof(NICSpec));
	if(!vm->nics) {
		failed();
		return -10;
	}
	for(int i = 0; i < vm->nic_count; i++) {
		READ2(read_uint64(rpc, &vm->nics[i].mac), failed);
		//READ2(read_uint32(rpc, &vm->nics[i].port), failed);
		char* ch;
		uint16_t len2;
		READ2(read_string(rpc, &ch, &len2), failed);
		vm->nics[i].dev = (char*)malloc(len2 + 1);
		bzero(vm->nics[i].dev, len2 + 1);
		memcpy(vm->nics[i].dev, ch, len2);

		READ2(read_uint32(rpc, &vm->nics[i].input_buffer_size), failed);
		READ2(read_uint32(rpc, &vm->nics[i].output_buffer_size), failed);
		READ2(read_uint64(rpc, &vm->nics[i].input_bandwidth), failed);
		READ2(read_uint64(rpc, &vm->nics[i].output_bandwidth), failed);
		READ2(read_uint8(rpc, &vm->nics[i].padding_head), failed);
		READ2(read_uint8(rpc, &vm->nics[i].padding_tail), failed);
		READ2(read_uint32(rpc, &vm->nics[i].pool_size), failed);
	}

	READ2(read_uint16(rpc, &vm->argc), failed);

	int rbuf_read = rpc->rbuf_read;
	int argv_size = sizeof(char**) * vm->argc;
	for(int i = 0; i < vm->argc; i++) {
		uint16_t len2;
		READ2(read_string(rpc, NULL, &len2), failed);

		argv_size += len2 + 1;
	}

	vm->argv = malloc(argv_size);
	if(!vm->argv) {
		failed();
		return -2;
	}
	bzero(vm->argv, argv_size);
	char* str = (void*)vm->argv + sizeof(char**) * vm->argc;

	rpc->rbuf_read = rbuf_read;
	for(int i = 0; i < vm->argc; i++) {
		char* ch;
		uint16_t len2;
		READ2(read_string(rpc, &ch, &len2), failed);

		vm->argv[i] = str;
		memcpy(str, ch, len2);
		str += len2 + 1;
	}

	*vm2 = vm;

	RETURN();
}

static int rbuf_flush(RPC* rpc) {
	int len = rpc->rbuf_read;
	memmove(rpc->rbuf, rpc->rbuf + len, rpc->rbuf_index - len);

	rpc->rbuf_index -= len;
	rpc->rbuf_read = 0;

	return len;
}

static int wbuf_flush(RPC* rpc) {
	int len = rpc->write(rpc, rpc->wbuf, rpc->wbuf_index);
	if(len < 0) {
		return len;
	}
	memmove(rpc->wbuf, rpc->wbuf + len, rpc->wbuf_index - len);

	rpc->wbuf_index -= len;

	return len;
}

// API
// hello client API
int rpc_hello(RPC* rpc, bool(*callback)(void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_HELLO_REQ));
	WRITE(write_string(rpc, RPC_MAGIC));
	WRITE(write_uint32(rpc, RPC_VERSION));

	rpc->hello_callback = callback;
	rpc->hello_context = context;

	RETURN();
}

static int hello_res_handler(RPC* rpc) {
	if(rpc->hello_callback && !rpc->hello_callback(rpc->hello_context)) {
		rpc->hello_callback = NULL;
		rpc->hello_context = NULL;
	}

	return 1;
}

// hello server API
static int hello_req_handler(RPC* rpc) {
	INIT();

	char* magic;
	uint16_t len;
	READ(read_string(rpc, &magic, &len));
	if(len != RPC_MAGIC_SIZE) {
		return -1;
	}

	if(memcmp(magic, RPC_MAGIC, RPC_MAGIC_SIZE) != 0) {
		return -1;
	}

	uint32_t ver;
	READ(read_uint32(rpc, &ver));
	if(ver == RPC_VERSION) {
		rpc->ver = ver;
	} else {
		return -1;
	}

	WRITE(write_uint16(rpc, RPC_TYPE_HELLO_RES));

	RETURN();
}

// vm_create client API
int rpc_vm_create(RPC* rpc, VMSpec* vm, bool(*callback)(uint32_t id, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_VM_CREATE_REQ));
	WRITE(write_vm(rpc, vm));

	rpc->vm_create_callback = callback;
	rpc->vm_create_context = context;

	RETURN();
}

static int vm_create_res_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	if(rpc->vm_create_callback && !rpc->vm_create_callback(id, rpc->vm_create_context)) {
		rpc->vm_create_callback = NULL;
		rpc->vm_create_context = NULL;
	}

	RETURN();
}

// vm_create server API
void rpc_vm_create_handler(RPC* rpc, void(*handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, uint32_t id)), void* context) {
	rpc->vm_create_handler = handler;
	rpc->vm_create_handler_context = context;
}


static void vm_create_handler_callback(RPC* rpc, uint32_t id) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_VM_CREATE_RES));
	WRITE2(write_uint32(rpc, id));

	RETURN2();
}

static int vm_create_req_handler(RPC* rpc) {
	INIT();

	VMSpec* vm;
	READ(read_vm(rpc, &vm));

	if(rpc->vm_create_handler) {
		rpc->vm_create_handler(rpc, vm, rpc->vm_create_handler_context, vm_create_handler_callback);
	} else {
		vm_create_handler_callback(rpc, 0);
	}

	if(vm)
		vm_free(vm);

	RETURN();
}

// vm_get client API
int rpc_vm_get(RPC* rpc, uint32_t id, bool(*callback)(VMSpec* vm, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_VM_GET_REQ));
	WRITE(write_uint32(rpc, id));

	rpc->vm_get_callback = callback;
	rpc->vm_get_context = context;

	RETURN();
}

static int vm_get_res_handler(RPC* rpc) {
	INIT();

	VMSpec* vm;
	READ(read_vm(rpc, &vm));

	if(rpc->vm_get_callback && !rpc->vm_get_callback(vm, rpc->vm_get_context)) {
		rpc->vm_get_callback = NULL;
		rpc->vm_get_context = NULL;
	}

	if(vm)
		vm_free(vm);

	RETURN();
}

// vm_get server API
void rpc_vm_get_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMSpec* vm)), void* context) {
	rpc->vm_get_handler = handler;
	rpc->vm_get_handler_context = context;
}

static void vm_get_handler_callback(RPC* rpc, VMSpec* vm) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_VM_GET_RES));
	WRITE2(write_vm(rpc, vm));

	RETURN2();
}

static int vm_get_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	if(rpc->vm_get_handler) {
		rpc->vm_get_handler(rpc, id, rpc->vm_get_handler_context, vm_get_handler_callback);
	} else {
		vm_get_handler_callback(rpc, NULL);
	}

	RETURN();
}

// vm_set client API
int rpc_vm_set(RPC* rpc, VMSpec* vm, bool(*callback)(bool result, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_VM_SET_REQ));
	WRITE(write_vm(rpc, vm));

	rpc->vm_set_callback = callback;
	rpc->vm_set_context = context;

	RETURN();
}

static int vm_set_res_handler(RPC* rpc) {
	INIT();

	bool result;
	READ(read_bool(rpc, &result));

	if(rpc->vm_set_callback && !rpc->vm_set_callback(result, rpc->vm_set_context)) {
		rpc->vm_set_callback = NULL;
		rpc->vm_set_context = NULL;
	}

	RETURN();
}

// vm_set server API
void rpc_vm_set_handler(RPC* rpc, void(*handler)(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, bool result)), void* context) {
	rpc->vm_set_handler = handler;
	rpc->vm_set_handler_context = context;
}

static void vm_set_handler_callback(RPC* rpc, bool result) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_VM_SET_RES));
	WRITE2(write_bool(rpc, result));

	RETURN2();
}

static int vm_set_req_handler(RPC* rpc) {
	INIT();

	VMSpec* vm;
	READ(read_vm(rpc, &vm));

	if(rpc->vm_set_handler) {
		rpc->vm_set_handler(rpc, vm, rpc->vm_set_handler_context, vm_set_handler_callback);
	} else {
		vm_set_handler_callback(rpc, false);
	}

	if(vm)
		vm_free(vm);

	RETURN();
}

// vm_destroy client API
int rpc_vm_destroy(RPC* rpc, uint32_t id, bool(*callback)(bool result, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_VM_DELETE_REQ));
	WRITE(write_uint32(rpc, id));

	rpc->vm_destroy_callback = callback;
	rpc->vm_destroy_context = context;

	RETURN();
}

static int vm_destroy_res_handler(RPC* rpc) {
	INIT();

	bool result = false;
	READ(read_bool(rpc, &result));

	if(rpc->vm_destroy_callback && !rpc->vm_destroy_callback(result, rpc->vm_destroy_context)) {
		rpc->vm_destroy_callback = NULL;
		rpc->vm_destroy_context = NULL;
	}

	RETURN();
}

// vm_destroy server API
void rpc_vm_destroy_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, bool result)), void* context) {
	rpc->vm_destroy_handler = handler;
	rpc->vm_destroy_handler_context = context;
}

static void vm_destroy_handler_callback(RPC* rpc, bool result) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_VM_DELETE_RES));
	WRITE2(write_bool(rpc, result));

	RETURN2();
}

static int vm_destroy_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	if(rpc->vm_destroy_handler) {
		rpc->vm_destroy_handler(rpc, id, rpc->vm_destroy_handler_context, vm_destroy_handler_callback);
	} else {
		vm_destroy_handler_callback(rpc, false);
	}

	RETURN();
}

// vm_list client API
int rpc_vm_list(RPC* rpc, bool(*callback)(uint32_t* ids, uint16_t count, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_VM_LIST_REQ));

	rpc->vm_list_callback = callback;
	rpc->vm_list_context = context;

	RETURN();
}

static int vm_list_res_handler(RPC* rpc) {
	INIT();

	int32_t size;
	uint32_t* list;
	READ(read_bytes(rpc, (void**)&list, &size));

	if(rpc->vm_list_callback && !rpc->vm_list_callback(list, (size < 0 ? 0 : size) / sizeof(uint32_t), rpc->vm_list_context)) {
		rpc->vm_list_callback = NULL;
		rpc->vm_list_context = NULL;
	}

	RETURN();
}

// vm_list server API
void rpc_vm_list_handler(RPC* rpc, void(*handler)(RPC* rpc, int size, void* context, void(*callback)(RPC* rpc, uint32_t* ids, int size)), void* context) {
	rpc->vm_list_handler = handler;
	rpc->vm_list_handler_context = context;
}

static void vm_list_handler_callback(RPC* rpc, uint32_t* ids, int size) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_VM_LIST_RES));
	WRITE2(write_bytes(rpc, ids, sizeof(uint32_t) * size));

	RETURN2();
}

static int vm_list_req_handler(RPC* rpc) {
	INIT();

	_size++;	// To avoid rollback

	if(rpc->vm_list_handler) {
		rpc->vm_list_handler(rpc, 255, rpc->vm_list_handler_context, vm_list_handler_callback);
	} else {
		vm_list_handler_callback(rpc, NULL, 0);
	}

	RETURN();
}

// status_get client API
int rpc_status_get(RPC* rpc, uint32_t id, bool(*callback)(VMStatus status, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_STATUS_GET_REQ));
	WRITE(write_uint32(rpc, id));

	rpc->status_get_callback = callback;
	rpc->status_get_context = context;

	RETURN();
}

static int status_get_res_handler(RPC* rpc) {
	INIT();

	uint32_t status;
	READ(read_uint32(rpc, &status));

	if(rpc->status_get_callback && !rpc->status_get_callback(status, rpc->status_get_context)) {
		rpc->status_get_callback = NULL;
		rpc->status_get_context = NULL;
	}

	RETURN();
}

// md5 server API
void rpc_storage_md5_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[])), void* context) {
	rpc->storage_md5_handler = handler;
	rpc->storage_md5_handler_context = context;
}

static void storage_md5_handler_callback(RPC* rpc, bool result, uint32_t md5[]) {
		INIT2();

		WRITE2(write_uint16(rpc, RPC_TYPE_STORAGE_MD5_RES));
		WRITE2(write_bool(rpc, result));
		for(int i = 0; i < 4; i++) {
			if(md5) {
				WRITE2(write_uint32(rpc, md5[i]));
			} else {
				WRITE2(write_uint32(rpc, 0));
			}
		}

		RETURN2();
	}

static int storage_md5_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	uint64_t size;
	READ(read_uint32(rpc, &id));
	READ(read_uint64(rpc, &size));

	if(rpc->status_get_handler) {
		rpc->storage_md5_handler(rpc, id, size, rpc->storage_md5_context, storage_md5_handler_callback);
	} else {
		storage_md5_handler_callback(rpc, false, NULL);
	}

	RETURN();
}

// md5 client API
void rpc_storage_md5(RPC* rpc, uint32_t id, uint64_t size, bool(*callback)(bool result, uint32_t md5[], void* context), void* context) {
	INIT();

	WRITE2(write_uint16(rpc, RPC_TYPE_STORAGE_MD5_REQ));
	WRITE2(write_uint32(rpc, id));
	WRITE2(write_uint64(rpc, size));

	rpc->storage_md5_callback = callback;
	rpc->storage_md5_context = context;

	RETURN2();
}

static int storage_md5_res_handler(RPC* rpc) {
	INIT();

	bool result;
	READ(read_bool(rpc, &result));

	uint32_t md5[4];
	for(int i = 0; i < 4; i++) {
		READ(read_uint32(rpc, &md5[i]));
	}

	if(rpc->storage_md5_callback && !rpc->storage_md5_callback(result, md5, rpc->storage_md5_context)) {
		rpc->storage_md5_callback = NULL;
		rpc->storage_md5_context = NULL;
	}

	RETURN();
}

// status_get server API
void rpc_status_get_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC* rpc, VMStatus status)), void* context) {
	rpc->status_get_handler = handler;
	rpc->status_get_handler_context = context;
}

static void status_get_handler_callback(RPC* rpc, VMStatus status) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_STATUS_GET_RES));
	WRITE2(write_uint32(rpc, status));

	RETURN2();
}

static int status_get_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	if(rpc->status_get_handler) {
		rpc->status_get_handler(rpc, id, rpc->status_get_handler_context, status_get_handler_callback);
	} else {
		status_get_handler_callback(rpc, VM_STATUS_INVALID);
	}

	RETURN();
}

// status_set client API
int rpc_status_set(RPC* rpc, uint32_t id, VMStatus status, bool(*callback)(bool result, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_STATUS_SET_REQ));
	WRITE(write_uint32(rpc, id));
	WRITE(write_uint32(rpc, (uint32_t)status));

	rpc->status_set_callback = callback;
	rpc->status_set_context = context;

	RETURN();
}

static int status_set_res_handler(RPC* rpc) {
	INIT();

	bool result = false;
	READ(read_bool(rpc, &result));

	if(rpc->status_set_callback && !rpc->status_set_callback(result, rpc->status_set_context)) {
		rpc->status_set_callback = NULL;
		rpc->status_set_context = NULL;
	}

	RETURN();
}

// status_set server API
void rpc_status_set_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result)), void* context) {
	rpc->status_set_handler = handler;
	rpc->status_set_handler_context = context;
}

static void status_set_handler_callback(RPC* rpc, bool result) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_STATUS_SET_RES));
	WRITE2(write_bool(rpc, result));

	RETURN2();
}

static int status_set_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id))

	uint32_t status;
	READ(read_uint32(rpc, &status));

	if(rpc->status_set_handler) {
		rpc->status_set_handler(rpc, id, status, rpc->status_set_handler_context, status_set_handler_callback);
	} else {
		status_set_handler_callback(rpc, false);
	}

	RETURN();
}

// storage_download client API
int rpc_storage_download(RPC* rpc, uint32_t id, uint64_t size, int32_t(*callback)(uint32_t offset, void* buf, int32_t size, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_STORAGE_DOWNLOAD_REQ));
	WRITE(write_uint32(rpc, id));
	WRITE(write_uint64(rpc, size));

	rpc->storage_download_callback = callback;
	rpc->storage_download_context = context;

	RETURN();
}

static int storage_download_res_handler(RPC* rpc) {
	INIT();

	uint32_t offset;
	READ(read_uint32(rpc, &offset));

	void* buf;
	int32_t size;
	READ(read_bytes(rpc, &buf, &size));

	if(rpc->storage_download_callback) {
		int size2 = rpc->storage_download_callback(offset, buf, size, rpc->storage_download_context);

		if(size <= 0 || size2 <= 0) {
			rpc->storage_download_callback = NULL;
			rpc->storage_download_context = NULL;
		}
	}

	RETURN();
}

// storage_download server API
void rpc_storage_download_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size)), void* context) {
	rpc->storage_download_handler = handler;
	rpc->storage_download_handler_context = context;
}

static int storage_download_req_handler(RPC* rpc) {
	INIT();

	READ(read_uint32(rpc, &rpc->storage_download_id));
	READ(read_uint64(rpc, &rpc->storage_download_size));

	rpc->storage_download_offset = 0;

	RETURN();
}

static void storage_download_handler_callback(RPC* rpc, void* buf, int32_t size) {
	INIT2();

	if(size > 0) {
		WRITE2(write_uint16(rpc, RPC_TYPE_STORAGE_DOWNLOAD_RES));
		WRITE2(write_uint32(rpc, rpc->storage_download_offset));
		WRITE2(write_bytes(rpc, buf, size));

		rpc->storage_download_offset += size;
	} else {
		WRITE2(write_uint16(rpc, RPC_TYPE_STORAGE_DOWNLOAD_RES));
		WRITE2(write_uint32(rpc, rpc->storage_download_offset));
		WRITE2(write_bytes(rpc, NULL, size));

		rpc->storage_download_id = 0;
		rpc->storage_download_offset = 0;
	}

	RETURN2();
}

static int download(RPC* rpc) {
	INIT();

	_size++;	// To avoid rollback

	if(rpc->storage_download_handler) {
		rpc->storage_download_handler(rpc, rpc->storage_download_id, rpc->storage_download_size, rpc->storage_download_offset, 1460, rpc->storage_download_handler_context, storage_download_handler_callback);
	} else {
		storage_download_handler_callback(rpc, NULL, -1);
	}

	RETURN();
}

// storage_upload client API
int rpc_storage_upload(RPC* rpc, uint32_t id, int32_t(*callback)(uint32_t offset, void** buf, int32_t size, void* context), void* context) {
	rpc->storage_upload_id = id;
	rpc->storage_upload_offset = 0;
	rpc->storage_upload_callback = callback;
	rpc->storage_upload_context = context;

	return 1;
}

static int storage_upload_res_handler(RPC* rpc) {
	INIT();

	int32_t size;
	READ(read_int32(rpc, &size));

	if(rpc->storage_upload_callback) {
		rpc->storage_upload_callback(rpc->storage_upload_offset, NULL, size, rpc->storage_upload_context);

		rpc->storage_upload_id = 0;
		rpc->storage_upload_offset = 0;
		rpc->storage_upload_callback = NULL;
		rpc->storage_upload_context = NULL;
	}

	RETURN();
}

static int upload(RPC* rpc) {
	INIT();

	void* buf;
	int32_t size = 0;
	if(rpc->storage_upload_callback) {
		size = rpc->storage_upload_callback(rpc->storage_upload_offset, &buf, 1460, rpc->storage_upload_context);

		WRITE(write_uint16(rpc, RPC_TYPE_STORAGE_UPLOAD_REQ));
		WRITE(write_uint32(rpc, rpc->storage_upload_id));
		WRITE(write_uint32(rpc, rpc->storage_upload_offset));
		WRITE(write_bytes(rpc, buf, size));

		if(size <= 0)
			rpc->storage_upload_offset = (uint32_t)-1;
	}

	rpc->storage_upload_offset += size;

	RETURN();
}

// storage_upload server API
void rpc_storage_upload_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size)), void* context) {
	rpc->storage_upload_handler = handler;
	rpc->storage_upload_handler_context = context;
}

void storage_upload_handler_callback(RPC* rpc, int32_t size) {
	if(size <= 0) {
		rpc->storage_upload_id = 0;
		rpc->storage_upload_offset = 0;

		INIT2();

		WRITE2(write_uint16(rpc, RPC_TYPE_STORAGE_UPLOAD_RES));
		WRITE2(write_int32(rpc, size));

		RETURN2();
	}
}

static int storage_upload_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	uint32_t offset;
	READ(read_uint32(rpc, &offset));

	void* buf;
	int32_t size;
	READ(read_bytes(rpc, &buf, &size));

	if(rpc->storage_upload_handler) {
		rpc->storage_upload_handler(rpc, id, offset, buf, size, rpc->storage_upload_handler_context, storage_upload_handler_callback);
	} else {
		storage_upload_handler_callback(rpc, -1);
	}

	RETURN();
}

// stdio client API
int rpc_stdio(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, const char* str, uint16_t size, bool(*callback)(uint16_t written, void* context), void* context) {
	INIT();

	WRITE(write_uint16(rpc, RPC_TYPE_STDIO_REQ));
	WRITE(write_uint32(rpc, id));
	WRITE(write_uint8(rpc, thread_id));
	WRITE(write_uint8(rpc, fd));
	WRITE(write_bytes(rpc, (void*)str, size));

	rpc->stdio_callback = callback;
	rpc->stdio_context = context;

	RETURN();
}

static int stdio_res_handler(RPC* rpc) {
	INIT();

	uint16_t written;
	READ(read_uint16(rpc, &written));

	if(rpc->stdio_callback && !rpc->stdio_callback(written, rpc->stdio_context)) {
		rpc->stdio_callback = NULL;
		rpc->stdio_context = NULL;
	}

	RETURN();
}

// stdio server API
void rpc_stdio_handler(RPC* rpc, void(*handler)(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)), void* context) {
	rpc->stdio_handler = handler;
	rpc->stdio_handler_context = context;
}

static void stdio_handler_callback(RPC* rpc, uint16_t size) {
	INIT2();

	WRITE2(write_uint16(rpc, RPC_TYPE_STDIO_RES));
	WRITE2(write_uint16(rpc, size));

	RETURN2();
}

static int stdio_req_handler(RPC* rpc) {
	INIT();

	uint32_t id;
	READ(read_uint32(rpc, &id));

	uint8_t thread_id;
	READ(read_uint8(rpc, &thread_id));

	uint8_t fd;
	READ(read_uint8(rpc, &fd));

	char* str;
	int32_t len;
	READ(read_bytes(rpc, (void**)&str, &len));

	if(rpc->stdio_handler) {
		rpc->stdio_handler(rpc, id, thread_id, fd, str, len, rpc->stdio_handler_context, stdio_handler_callback);
	} else {
		stdio_handler_callback(rpc, 0);
	}

	RETURN();
}

// Handlers
typedef int(*Handler)(RPC*);

static Handler handlers[] = {
	NULL,
	hello_req_handler,
	hello_res_handler,
	vm_create_req_handler,
	vm_create_res_handler,
	vm_get_req_handler,
	vm_get_res_handler,
	vm_set_req_handler,
	vm_set_res_handler,
	vm_destroy_req_handler,
	vm_destroy_res_handler,
	vm_list_req_handler,
	vm_list_res_handler,
	status_get_req_handler,
	status_get_res_handler,
	status_set_req_handler,
	status_set_res_handler,
	storage_download_req_handler,
	storage_download_res_handler,
	storage_upload_req_handler,
	storage_upload_res_handler,
	storage_md5_req_handler,
	storage_md5_res_handler,
	stdio_req_handler,
	stdio_res_handler,
	download,
	upload,
};

bool rpc_is_active(RPC* rpc) {
	return rpc->storage_download_id > 0 || rpc->storage_upload_id > 0 || rpc->wbuf_index > 0;
}

bool rpc_loop(RPC* rpc) {
	if(rpc->wbuf_index > 0 && rpc->write) {
		if(wbuf_flush(rpc) < 0 && rpc->close) {
			rpc->close(rpc);
			return false;
		}
	}

	bool is_first = true;
	while(true) {
		INIT();

		uint16_t type = (uint16_t)-1;
		_len = read_uint16(rpc, &type);

		if(_len > 0) {
			if(type >= RPC_TYPE_END || !handlers[type]) {
				if(rpc->close)
					rpc->close(rpc);

				return _size > 0;
			}
		} else if(_len < 0) {
			if(rpc->close)
				rpc->close(rpc);

			return _size > 0;
		} else {
			if(!is_first)
				return _size > 0;

			if(rpc->storage_download_id > 0) {
				type = RPC_TYPE_END;	// download
			} else if(rpc->storage_upload_id > 0 && rpc->storage_upload_offset != (uint32_t)-1) {
				type = RPC_TYPE_END + 1;// upload
			}
		}

		if(type != (uint16_t)-1) {
			_len = handlers[type](rpc);
			if(_len > 0) {
				_size += _len;

				if(rpc->wbuf_index > 0 && rpc->write && wbuf_flush(rpc) < 0 && rpc->close) {
					rpc->close(rpc);
					return false;
				}

				if(rbuf_flush(rpc) < 0 && rpc->close) {
					rpc->close(rpc);
					return false;
				}
			} else if(_len == 0) {
				ROLLBACK();
				return _size > 0;
			} else if(rpc->close) {
				rpc->close(rpc);
				return _size > 0;
			}
		} else {
			return _size > 0;
		}

		is_first = false;
	}
}

static void vm_free(VMSpec* vm) {
	if(vm->argv)
		free(vm->argv);

	if(vm->nics)
		free(vm->nics);

	free(vm);
}

void rpc_vm_dump(VMSpec* vm) {
	if(vm == NULL) {
		printf("vm = %p\n", vm);
		return;
	}

	printf("id = %d\n", vm->id);
	printf("core_size = %d\n", vm->core_size);
	printf("memory_size = %x\n", vm->memory_size);
	printf("storage_size = %x\n", vm->storage_size);
	for(int i = 0; i < vm->nic_count; i++) {
		printf("\tnic[%d].mac = %lx\n", i, vm->nics[i].mac);
		printf("\tnic[%d].dev = %s\n", i, vm->nics[i].dev);
		printf("\tnic[%d].input_buffer_size = %d\n", i, vm->nics[i].input_buffer_size);
		printf("\tnic[%d].output_buffer_size = %d\n", i, vm->nics[i].output_buffer_size);
		printf("\tnic[%d].input_bandwidth = %lx\n", i, vm->nics[i].input_bandwidth);
		printf("\tnic[%d].output_bandwidth = %lx\n", i, vm->nics[i].output_bandwidth);
		printf("\tnic[%d].pool_size = %x\n", i, vm->nics[i].pool_size);
	}
	printf("argv: ");
	for(int i = 0; i < vm->argc; i++) {
		if(strchr(vm->argv[i], ' ')) {
			printf("\"%s\" ", vm->argv[i]);
		} else {
			printf("%s ", vm->argv[i]);
		}
	}
	printf("\n");
}

#ifdef LINUX
#define DEBUG 0
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

typedef struct {
	int	fd;
} RPCData;

static int sock_read(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = recv(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Read: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len == -1) {
		return 0;
	} else if(len == 0) {
		return -1;
	} else {
		return len;
	}
}

static int sock_write(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = send(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Write: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len == -1) {
		return 0;
	} else if(len == 0) {
		return -1;
	} else {
		return len;
	}
}

static void sock_close(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	close(data->fd);
	data->fd = -1;
}

RPC* rpc_open(const char* host, int port, int timeout) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		return NULL;
	}

	void handler(int signo) {
		// Do nothing just interrupt
	}

	struct sigaction sigact, old_sigact;
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_INTERRUPT;

	if(sigaction(SIGALRM, &sigact, &old_sigact) < 0) {
		close(fd);
		return NULL;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	alarm(timeout);

	if(connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		alarm(0);
		sigaction(SIGALRM, &old_sigact, NULL);
		close(fd);
		return NULL;
	}

	alarm(0);

	if(sigaction(SIGALRM, &old_sigact, NULL) < 0) {
		close(fd);
		return NULL;
	}

	RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
	rpc->read = sock_read;
	rpc->write = sock_write;
	rpc->close = sock_close;

	RPCData* data = (RPCData*)rpc->data;
	data->fd = fd;

	return rpc;
}

RPC* rpc_listen(int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		return NULL;
	}

	bool reuse = true;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		return NULL;
	}

	RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
	bzero(rpc, sizeof(RPC));
	RPCData* data = (RPCData*)rpc->data;
	data->fd = fd;

	return rpc;
}

RPC* rpc_accept(RPC* srpc) {
	RPCData* data = (RPCData*)srpc->data;

	if(listen(data->fd, 5) < 0) {
		return NULL;
	}

	struct sockaddr_in caddr;
	socklen_t len = sizeof(struct sockaddr_in);
	int fd = accept(data->fd, (struct sockaddr*)&caddr, &len);
	if(fd < 0) {
		return NULL;
	}

	RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
	memcpy(rpc, srpc, sizeof(RPC));
	rpc->ver = 0;
	rpc->rbuf_read = 0;
	rpc->rbuf_index = 0;
	rpc->wbuf_index = 0;
	rpc->read = sock_read;
	rpc->write = sock_write;
	rpc->close = sock_close;

	data = (RPCData*)rpc->data;
	data->fd = fd;

	return rpc;
}

bool rpc_is_closed(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	return data->fd < 0;

}
#endif /* LINUX */
