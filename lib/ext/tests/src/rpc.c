#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <control/rpc.h>

char* ip = "192.168.0.3";

int server_open = 0;
void* server(void* arg) {
	int port = *(int*)arg;
	int server_sock = 0;
	struct sockaddr_in server_addr;	

	if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
		perror("server socket faield\n");
		exit(-1);
	}	

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	
	if(bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind failed\n");
		exit(-1);
	}

	if(listen(server_sock, 3) < 0) {
		perror("listen failed\n");
		exit(-1);
	}

	while(!server_open);

	close(server_sock);

	return NULL;
}

static void rpc_open_func() {
	/* open tcp server by thread */
	pthread_t tcp_server;
	int port = 1024;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* test = rpc_open(ip, 1024, 1);
	assert_non_null(test);

	uint8_t fd = *test->data;

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	socklen_t len = sizeof(sock_addr);
	if(getpeername(fd, (struct sockaddr*)&sock_addr, &len) < 0) {
		perror("getsockname failed\n");
		exit(-1);
	}
	
	assert_int_equal(sock_addr.sin_addr.s_addr, inet_addr("192.168.0.3"));	
	assert_int_equal(ntohs(sock_addr.sin_port), ntohs(htons(1024)));	

	server_open = 1;
}

static void rpc_listen_func() {
	RPC* rpc = rpc_listen(2048);

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	socklen_t len = sizeof(sock_addr);
	uint8_t fd = rpc->data[0];
	if(getsockname(fd, (struct sockaddr*)&sock_addr, &len) < 0) {
		perror("getsockname failed\n");
		exit(-1);
	}

	assert_int_equal(ntohs(sock_addr.sin_port), 2048);
}

int client_open = 0;
void* client(void* arg) {
	int port = *(int*)arg;
	int sock_fd = 0;
	struct sockaddr_in sock_addr;
	socklen_t len = sizeof(sock_addr);
	memset(&sock_addr, 0, len);

	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("sock socket failed\n");
		exit(-1);
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr("192.168.0.3"); 
	sock_addr.sin_port = htons(port);

	sleep(1);
	if(connect(sock_fd, (struct sockaddr*)&sock_addr, len) < 0) {
		perror("sock connect failed\n");
		exit(-1);
	}	

	close(sock_fd);			

	return NULL;
}

static void rpc_accept_func() {
	int port = 4096;
	RPC* listen_rpc = rpc_listen(port);				
	pthread_t tcp_client;
	if(pthread_create(&tcp_client, NULL, &client, &port) < 0) {
		perror("rcp accept thread create failed\n");
		exit(-1);
	}
	
	RPC* accept_rpc = rpc_accept(listen_rpc);

	int rtn = 0;

	int listen_port = 0;
	int accept_port = 0;

	struct sockaddr_in test;
	socklen_t len = sizeof(test);
	memset(&test, 0, len);
	rtn = getsockname(listen_rpc->data[0], (struct sockaddr*)&test, &len);	
	if(rtn < 0) {
		perror("getsockname failed\n");
		exit(-1);
	}
	listen_port = ntohs(test.sin_port);

	memset(&test, 0, len);
	rtn = getsockname(accept_rpc->data[0], (struct sockaddr*)&test, &len);	
	if(rtn < 0) {
		perror("getsockname failed\n");
		exit(-1);
	}
	accept_port = ntohs(test.sin_port);

	assert_int_equal(listen_port, accept_port);
	assert_int_equal(test.sin_addr.s_addr, inet_addr("192.168.0.3"));

	int status;
	pthread_join(tcp_client, (void**)&status);
}

static void rpc_is_closed_func() {
	server_open = 0;

	pthread_t tcp_server;
	int port = 9012;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* test = rpc_open("192.168.0.3", port, 1);
	if(!test) {
		perror("rpc open failed\n");
		exit(-1);
	}
	
	int rtn = close(test->data[0]);
	bool result = rpc_is_closed(test);					
	//assert_int_equal(true, result);

	server_open = 1;

	int status;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_hello_func() {
	server_open = 0;
	int port = 1028;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_hello(rpc, NULL, NULL);
	
	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_HELLO_REQ);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 2), strlen(RPC_MAGIC));
	assert_memory_equal((char*)(rpc->wbuf + 2 + 2), RPC_MAGIC, strlen(RPC_MAGIC));
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 2 + 2 + 5), RPC_VERSION);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_create_func() {
	server_open = 0;
	int port = 1028;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	VMSpec vm;
	vm.id = 1;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 1;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 1;
	vm.nic_count = 1;
	char* _args[VM_MAX_ARGC];
	_args[0] = "Test";
	vm.argv = _args;
	char* dev = "eth0";

	nics[0].mac = 0x00000001;
	nics[0].dev = dev;
	nics[0].input_buffer_size = 1024;
	nics[0].output_buffer_size = 1024;
	nics[0].input_bandwidth = 1000000000;
	nics[0].output_bandwidth = 1000000000;
	nics[0].pool_size = 0x400000;
	nics[0].padding_head = 1;
	nics[0].padding_tail = 1;

	rpc_vm_create(rpc, &vm, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_VM_CREATE_REQ);
	assert_int_equal(*(bool*)(rpc->wbuf + 2), true);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 3), vm.id);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 7), vm.core_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 11), vm.memory_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 15), vm.storage_size);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 19), vm.nic_count);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 21), nics[0].mac);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 29), strlen(dev));
	assert_memory_equal((char*)(rpc->wbuf + 31), nics[0].dev, strlen(dev));
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 35), nics[0].input_buffer_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 39), nics[0].output_buffer_size);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 43), nics[0].input_bandwidth);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 51), nics[0].output_bandwidth);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 59), nics[0].padding_head);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 60), nics[0].padding_tail);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 61), nics[0].pool_size);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 65), vm.argc);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 67), strlen("Test"));
	assert_memory_equal((char*)(rpc->wbuf + 69), vm.argv[0], strlen("Test"));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_get_func() {
	server_open = 0;
	int port = 1028;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	rpc_vm_get(rpc, 1, NULL, NULL);
	
	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_VM_GET_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_set_func() {
	server_open = 0;
	int port = 1029;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	VMSpec vm;
	vm.id = 1;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 1;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 1;
	vm.nic_count = 1;
	char* _args[VM_MAX_ARGC];
	_args[0] = "Test";
	vm.argv = _args;
	char* dev = "eth0";

	nics[0].mac = 0x00000001;
	nics[0].dev = dev;
	nics[0].input_buffer_size = 1024;
	nics[0].output_buffer_size = 1024;
	nics[0].input_bandwidth = 1000000000;
	nics[0].output_bandwidth = 1000000000;
	nics[0].pool_size = 0x400000;
	nics[0].padding_head = 1;
	nics[0].padding_tail = 1;

	rpc_vm_set(rpc, &vm, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_VM_SET_REQ);
	assert_int_equal(*(bool*)(rpc->wbuf + 2), true);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 3), vm.id);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 7), vm.core_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 11), vm.memory_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 15), vm.storage_size);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 19), vm.nic_count);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 21), nics[0].mac);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 29), strlen(dev));
	assert_memory_equal((char*)(rpc->wbuf + 31), nics[0].dev, strlen(dev));
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 35), nics[0].input_buffer_size);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 39), nics[0].output_buffer_size);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 43), nics[0].input_bandwidth);
	assert_int_equal(*(uint64_t*)(rpc->wbuf + 51), nics[0].output_bandwidth);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 59), nics[0].padding_head);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 60), nics[0].padding_tail);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 61), nics[0].pool_size);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 65), vm.argc);
	assert_int_equal(*(uint16_t*)(rpc->wbuf + 67), strlen("Test"));
	assert_memory_equal((char*)(rpc->wbuf + 69), vm.argv[0], strlen("Test"));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_destroy_func() {
	server_open = 0;
	int port = 1029;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_vm_destroy(rpc, 1, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_VM_DELETE_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_list_func() {
	server_open = 0;
	int port = 1030;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_vm_list(rpc, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_VM_LIST_REQ);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_status_get_func() {
	server_open = 0;
	int port = 1030;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_status_get(rpc, 1, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_STATUS_GET_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_download_func() {
	server_open = 0;
	int port = 1031;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_storage_download(rpc, 1, 1024, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_STORAGE_DOWNLOAD_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 6), 1024);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_upload_func() {
	server_open = 0;
	int port = 1031;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_storage_upload(rpc, 1, NULL, NULL);

	assert_int_equal(rpc->storage_upload_id, 1);
	assert_int_equal(rpc->storage_upload_offset, 0);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_md5_func() {
	server_open = 0;
	int port = 1031;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	
	rpc_storage_md5(rpc, 1, 1024, NULL, NULL);

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_STORAGE_MD5_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 6), 1024);

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_stdio_func() {
	server_open = 0;
	int port = 1031;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	char* str = "Test";
	rpc_stdio(rpc, 1, 2, 4, str, strlen(str), NULL, NULL);		

	assert_int_equal(*(uint16_t*)(rpc->wbuf), RPC_TYPE_STDIO_REQ);
	assert_int_equal(*(uint32_t*)(rpc->wbuf + 2), 1);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 6), 2);
	assert_int_equal(*(uint8_t*)(rpc->wbuf + 7), 4);
	assert_int_equal(*(int32_t*)(rpc->wbuf + 8), strlen(str));
	assert_memory_equal((char*)(rpc->wbuf + 12), str, strlen(str));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_create_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC*rpc, uint32_t id)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_vm_create_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->vm_create_handler);
	assert_memory_equal(context, rpc->vm_create_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_get_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC*rpc, VMSpec* vm)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_vm_get_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->vm_get_handler);
	assert_memory_equal(context, rpc->vm_get_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_set_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC*rpc, bool result)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_vm_set_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->vm_set_handler);
	assert_memory_equal(context, rpc->vm_set_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_destroy_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC*rpc, bool result)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_vm_destroy_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->vm_destroy_handler);
	assert_memory_equal(context, rpc->vm_destroy_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_vm_list_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, int size, void* context, void(*callback)(RPC*rpc, uint32_t* ids, int size)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_vm_list_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->vm_list_handler);
	assert_memory_equal(context, rpc->vm_list_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_status_get_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, void* context, void(*callback)(RPC*rpc, VMStatus status)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_status_get_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->status_get_handler);
	assert_memory_equal(context, rpc->status_get_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_status_set_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_status_set_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->status_set_handler);
	assert_memory_equal(context, rpc->status_set_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_download_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_storage_download_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->storage_download_handler);
	assert_memory_equal(context, rpc->storage_download_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_upload_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_storage_upload_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->storage_upload_handler);
	assert_memory_equal(context, rpc->storage_upload_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_storage_md5_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[])) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_storage_md5_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->storage_md5_handler);
	assert_memory_equal(context, rpc->storage_md5_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_stdio_handler_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);

	void handler(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)) {
		printf("Test\n");		
	}

	char* context = "Context";
	rpc_stdio_handler(rpc, handler, (void*)context);	

	assert_int_equal(handler, rpc->stdio_handler);
	assert_memory_equal(context, rpc->stdio_handler_context, strlen(context));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

static void rpc_is_active_func() {
	server_open = 0;
	int port = 1034;
	pthread_t tcp_server;

	server_open = 0;
	if(pthread_create(&tcp_server, NULL, &server, (void*)&port) < 0) {
		perror("thread created failed\n");
		exit(-1);
	}

	sleep(1);

	RPC* rpc = rpc_open(ip, port, 1);
	rpc->storage_download_id = 1;
	rpc->storage_upload_id = 1;
	rpc->wbuf_index = 1;
	assert_int_equal(true, rpc_is_active(rpc));

	rpc->storage_download_id = 1;
	rpc->storage_upload_id = 0;
	rpc->wbuf_index = 1;
	assert_int_equal(true, rpc_is_active(rpc));

	rpc->storage_download_id = 0;
	rpc->storage_upload_id = 0;
	rpc->wbuf_index = 0;
	assert_int_equal(false, rpc_is_active(rpc));

	server_open = 1;

	int status = 0;
	pthread_join(tcp_server, (void**)&status);
}

void* rpc_server(void* arg) {
	int port = *(int*)arg;
	RPC* rpc = rpc_listen(port);
	rpc = rpc_accept(rpc);	
	if(!rpc) {
		perror("RPC Accept Faield\n");
		exit(-1);
	}

	while(1) {
		rpc_loop(rpc);
	}	
}

int client_count = 0;
int server_count = 0;

char* rpc_hello_context = "HelloCallback";
bool rpc_hello_callback(void* context) {
	assert_memory_equal(context, rpc_hello_context, strlen(context));
	client_count--;
	return true;
}

uint32_t rpc_vm_create_id = 1;
char* rpc_vm_create_context = "VMCreate";
bool rpc_vm_create_callback(uint32_t id, void* context) {
	assert_memory_equal(context, rpc_vm_create_context, strlen(context));
	server_count--;
	return true;
}


void* rpc_client(void* arg) {
	int port = *(int*)arg;
	RPC* rpc;

	rpc = rpc_open(ip, port, 1);
	if(!rpc) {
		perror("RPC open Failed\n");
		exit(-1);
	}

	rpc_hello(rpc, rpc_hello_callback, (void*)rpc_hello_context);
	client_count++;

	VMSpec vm;
	vm.id = 1;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 1;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 1;
	vm.nic_count = 1;
	char* _args[VM_MAX_ARGC];
	_args[0] = "Test";
	vm.argv = _args;
	char* dev = "eth0";

	nics[0].mac = 0x00000001;
	nics[0].dev = dev;
	nics[0].input_buffer_size = 1024;
	nics[0].output_buffer_size = 1024;
	nics[0].input_bandwidth = 1000000000;
	nics[0].output_bandwidth = 1000000000;
	nics[0].pool_size = 0x400000;
	nics[0].padding_head = 1;
	nics[0].padding_tail = 1;
	rpc_vm_create(rpc, &vm, rpc_vm_create_callback, rpc_vm_create_context);
	client_count++;

	while(1) {
		rpc_loop(rpc);
	}
}

static void rpc_loop_func() {	
	pthread_t server;
	pthread_t client;

	int port = 3127;//rand() * 5000 + 1000;
	if(pthread_create(&server, NULL, &rpc_server, &port) < 0) {
		perror("Server Create Failed\n");	
		exit(-1);
	}

	if(pthread_create(&client, NULL, &rpc_client, &port) < 0) {
		perror("Client Create Failed\n");	
		exit(-1);
	}

	pthread_join(client, NULL);
	pthread_join(server, NULL);
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
//		cmocka_unit_test(rpc_open_func),
//		cmocka_unit_test(rpc_listen_func),
//		cmocka_unit_test(rpc_accept_func),
//		cmocka_unit_test(rpc_is_closed_func),
//		cmocka_unit_test(rpc_hello_func),
//		cmocka_unit_test(rpc_vm_create_func),
//		cmocka_unit_test(rpc_vm_get_func),
//		cmocka_unit_test(rpc_vm_set_func),
//		cmocka_unit_test(rpc_vm_destroy_func),
//		cmocka_unit_test(rpc_vm_list_func),
//		cmocka_unit_test(rpc_status_get_func),
//		cmocka_unit_test(rpc_storage_download_func),
//		cmocka_unit_test(rpc_storage_upload_func),
//		cmocka_unit_test(rpc_storage_md5_func),
//		cmocka_unit_test(rpc_stdio_func),
//		cmocka_unit_test(rpc_vm_create_handler_func),
//		cmocka_unit_test(rpc_vm_get_handler_func),
//		cmocka_unit_test(rpc_vm_set_handler_func),
//		cmocka_unit_test(rpc_vm_destroy_handler_func),
//		cmocka_unit_test(rpc_vm_list_handler_func),
//		cmocka_unit_test(rpc_vm_get_handler_func),
//		cmocka_unit_test(rpc_vm_set_handler_func),
//		cmocka_unit_test(rpc_storage_download_handler_func),
//		cmocka_unit_test(rpc_storage_upload_handler_func),
//		cmocka_unit_test(rpc_storage_upload_handler_func),
//		cmocka_unit_test(rpc_storage_upload_handler_func),
//		cmocka_unit_test(rpc_is_active_func),
		cmocka_unit_test(rpc_loop_func)

	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
