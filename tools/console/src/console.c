#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <util/list.h>
#include <util/cmd.h>
#include <util/types.h>
#include <control/rpc.h>
#include <control/vmspec.h>

#define VERSION		"0.1.0"
#define DEFAULT_HOST	"192.168.100.254"
#define DEFAULT_PORT	1111
#define MAX_LINE_SIZE	2048

#define ERROR_RPC_DISCONNECTED	-10000
#define ERROR_CMD_EXECUTE	-10001
#define ERROR_MALLOC_NULL	-10002

#define END_OF_FILE -1

static RPC* rpc;
static bool sync_status = true;
static bool is_continue = true;

static int cmd_exit(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	is_continue = false;

	return 0;
}

static int cmd_echo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	int pos = 0;

	for(int i = 1; i < argc; i++) {
		pos += sprintf(cmd_result + pos, "%s", argv[i]);
		if(i + 1 < argc)
			cmd_result[pos++] = ' ';
	}

	callback(cmd_result, 0);

	return 0;
}

static int cmd_sleep(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t time = 1;

	if(argc >= 2 && is_uint32(argv[1]))
		time = parse_uint32(argv[1]);

	sleep(time);

	return 0;
}

static void stdio_handler(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)) {
	static uint32_t current_id = -1;
	static uint8_t current_thread_id = -1;

	if(!(current_id == id && current_thread_id == thread_id)) {
		current_id = id;
		current_thread_id = thread_id;
		printf( "***** vmid=%d thread=%d standard %s *****\n", current_id, current_thread_id, fd == 1 ? "Output" : "Error");
		fflush(stdout);
	}

	size = write(1, str, size);
	fflush(stdout);
	
	callback(rpc, size);
}

typedef struct {
	uint16_t count;
	uint64_t current;
} HelloContext;

static bool callback_hello(void* context) {
	HelloContext* context_hello = context;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

	printf("Ping PacketNgin time = %0.3f ms\n", (float)(current - context_hello->current) / (float)1000);
	context_hello->current = current;

	context_hello->count--;

	if(context_hello->count == 0) {
		free(context_hello);
		sync_status = true;

		return false;
	}

	rpc_hello(rpc, callback_hello, context_hello);
	
	return true;
}

static int cmd_connect(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	char* host = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	
	if(argc >= 2)
		host = argv[1];
	
	if(argc >= 3 && is_uint16(argv[2]))
		port = parse_uint16(argv[2]);
	
	if(rpc) {
		rpc->close(rpc);
		rpc = NULL;
	}

	rpc = rpc_open(host, port, 3);
	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	HelloContext* context_hello = malloc(sizeof(HelloContext));
	if(context_hello == NULL)
		return ERROR_MALLOC_NULL;
	
	context_hello->count = 1;
	context_hello->current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

	rpc_hello(rpc, callback_hello, context_hello);

	rpc_stdio_handler(rpc, stdio_handler, NULL);
	sync_status = false;

	return 0;
}

static int cmd_ping(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	uint16_t count = 1;
	if(argc >= 2 && is_uint16(argv[1])) {
		count = parse_uint16(argv[1]);
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	HelloContext* context_hello = malloc(sizeof(HelloContext));
	if(context_hello == NULL)
		return ERROR_MALLOC_NULL;

	context_hello->current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
	context_hello->count = count;

	rpc_hello(rpc, callback_hello, context_hello);

	return 0;
}

static int cmd_vm_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_vm_create(uint32_t id, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(id == 0)
			callback("fail", -1);
		else {
			sprintf(cmd_result, "%d", id);
			callback(cmd_result, 0);
		}

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	VMSpec vm;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 0;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 0;
	char* _args[VM_MAX_ARGC];
	vm.argv = _args;
	
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "core:") == 0) {
			i++;
			if(!is_uint8(argv[i])) {
				printf("core must be uint8\n");
				return i;
			}
			
			vm.core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "memory:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("memory must be uint32\n");
				return i;
			}
			
			vm.memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "storage:") == 0) {
			i++;
			if(!is_uint32(argv[i])) {
				printf("storage must be uint32\n");
				return i;
			}
			
			vm.storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "nic:") == 0) {
			i++;
			
			NICSpec* nic = &vm.nics[vm.nic_count];
			vm.nic_count++;
			
			for( ; i < argc; i++) {
				if(strcmp(argv[i], "mac:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						if(!strcmp(argv[i], "dev_mac")) {
							nic->mac = NICSPEC_DEVICE_MAC;
							continue;
						} else {
							printf("mac must be uint64\n");
							return i;
						}
					} else 
						nic->mac = parse_uint64(argv[i]) & 0xffffffffffff;
				} else if(strcmp(argv[i], "dev:") == 0) {
					i++;
					nic->dev = argv[i];
				} else if(strcmp(argv[i], "ibuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("ibuf must be uint32\n");
						return i;
					}
					nic->input_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "obuf:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("obuf must be uint32\n");
						return i;
					}
					nic->output_buffer_size = parse_uint32(argv[i]);
				} else if(strcmp(argv[i], "iband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("iband must be uint64\n");
						return i;
					}
					nic->input_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "oband:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("oband must be uint64\n");
						return i;
					}
					nic->output_bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "hpad:") == 0) {
					i++;
					if(!is_uint8(argv[i])) {
						printf("hpad must be uint8\n");
						return i;
					}
					nic->padding_head = parse_uint8(argv[i]);
				} else if(strcmp(argv[i], "tpad:") == 0) {
					i++;
					if(!is_uint8(argv[i])) {
						printf("tpad must be uint8\n");
						return i;
					}
					nic->padding_tail = parse_uint8(argv[i]);
				} else if(strcmp(argv[i], "pool:") == 0) {
					i++;
					if(!is_uint32(argv[i])) {
						printf("pool must be uint32\n");
						return i;
					}
					nic->pool_size = parse_uint32(argv[i]);
				} else {
					i--;
					break;
				}
			}
		} else if(strcmp(argv[i], "args:") == 0) {
			i++;
			for( ; i < argc; i++)
				vm.argv[vm.argc++] = argv[i];
		}
	}
	
	rpc_vm_create(rpc, &vm, callback_vm_create, callback);
	sync_status = false;
	
	return 0;
}

static int cmd_vm_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_vm_destroy(bool result, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(result)
			callback("true", 0);
		else
			callback("false", -1);

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 2)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return 1;
	
	uint32_t vmid = parse_uint32(argv[1]);

	rpc_vm_destroy(rpc, vmid, callback_vm_destroy, callback);
	sync_status = false;

	return 0;
}

static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_vm_list(uint32_t* ids, uint16_t count, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		int pos = 0;
		for(int i = 0 ; i < count; i++)
			pos += sprintf(cmd_result + pos, "%d ", ids[i]);

		callback(cmd_result, 0);

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	rpc_vm_list(rpc, callback_vm_list, callback);
	sync_status = false; 
	return 0;
}	

//size's size & success or fail
typedef struct {
	char path[256];
	int fd;
	uint64_t file_size;
	uint32_t offset;
	uint64_t current_time;
	void(*callback)(char* result, int eixt_status);
} FileInfo;

static int cmd_upload(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	int32_t callback_storage_upload(uint32_t offset, void** buf, int32_t size, void* context) {
		FileInfo* file_info = context;
		static uint8_t ubuf[2048];
		
		if(size < 0) {
			printf("Storage Upload Error\n");
			fflush(stdout);

			close(file_info->fd);
			file_info->callback("false", -1);
			free(file_info);

			return 0;
		}

		if(size == 0) {
			printf("\nStorage Upload Completed\n");
			fflush(stdout);

			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
			printf("time = %0.3f ms\n", (float)(current - file_info->current_time) / (float)1000);

			close(file_info->fd);
			file_info->callback("true", 0);
			free(file_info);

			return 0;
		}

		if(offset == 0) {
			printf("Total Upload Size : %ld Byte\n", file_info->file_size);
		}
		
		if(file_info->offset != offset) {
			lseek(file_info->fd, offset, SEEK_SET);
			file_info->offset = offset;
		}
		
		size = read(file_info->fd, ubuf, size > (file_info->file_size - file_info->offset) ? (file_info->file_size - file_info->offset) : size);

		if(size < 0) {
			printf("\nFile Read Error\n");
			fflush(stdout);
			buf = NULL;

			return -1;
		}

		file_info->offset += size;
		*buf = ubuf;
		
		printf(".");
		fflush(stdout);

		return size;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 3 || argc > 4)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return -1;

	if(strlen(argv[2]) >= 256)
		return -2;

	if(argc == 4) {
		if(!is_uint32(argv[3]))
			return -3;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	
	FileInfo* file_info = malloc(sizeof(FileInfo));
	if(file_info == NULL) {
		return ERROR_MALLOC_NULL;
	}
	memset(file_info, 0, sizeof(FileInfo));

	file_info->fd = open(argv[2], O_RDONLY);
	if(file_info->fd < 0) {
		free(file_info);

		return -2;
	}

	file_info->file_size = lseek(file_info->fd, 0, SEEK_END);
	lseek(file_info->fd, 0, SEEK_SET);
	file_info->offset = 0;
	file_info->callback = callback;

	if(argc == 4) {
		uint32_t size = parse_uint32(argv[3]);
		if(size > file_info->file_size) {
			printf("File size is smaller than paramter\n");
			return -3;
		}
		file_info->file_size = size;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	file_info->current_time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

	rpc_storage_upload(rpc, vmid, callback_storage_upload, file_info);
	sync_status = false;

	return 0;
}

static int cmd_download(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	int callback_storage_download(uint32_t offset, void* buf, int32_t size, void* context) {
		FileInfo* file_info = context;

		if(size < 0) {
			printf("Storage Download Error\n");
			close(file_info->fd);
			file_info->callback("false", -1);
			remove(file_info->path);
			free(file_info);
			return 0;
		}

		if(file_info->offset != offset) {
			lseek(file_info->fd, offset, SEEK_SET);
			file_info->offset = offset;
		}

		size = write(file_info->fd, buf, size);
		file_info->offset += size;

		if(size < 0) {
			printf("Write Error!\n");
			close(file_info->fd);
			file_info->callback("false", -1);
			remove(file_info->path);
			free(file_info);
			return 0;
		}

		if(size == 0) {
			printf("\nStorage Download Completed\n");
			//uint32_t file_size = lseek(file_info->fd, 0, SEEK_CUR);
			printf("Total Size : %d Byte\n", file_info->offset);
			close(file_info->fd);

			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
			printf("time = %0.3f ms\n", (float)(current - file_info->current_time) / (float)1000);

			file_info->callback("true", 0);
			free(file_info);

			return 0;
		}

		printf(".");
		fflush(stdout);

		return size;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 3)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return -1;

	if(strlen(argv[2]) >= 256)
		return -2;

	uint32_t vmid = parse_uint32(argv[1]);
	
	FileInfo* file_info = (FileInfo*)malloc(sizeof(FileInfo));;
	if(file_info == NULL) {
		return ERROR_MALLOC_NULL;
	}
	memset(file_info, 0, sizeof(FileInfo));

	file_info->fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0755);
	
	if(file_info->fd < 0) {
		free(file_info);
		return -2;
	}

	strcpy(file_info->path, argv[2]);
	lseek(file_info->fd, 0, SEEK_SET);
	uint64_t download_size = 0;
	if(argc == 4) {
		if(!is_uint32(argv[3]))
			return -3;

		//file_info->file_size = parse_uint32(argv[3]);
		download_size = parse_uint32(argv[3]);
	} else {
		//file_info->file_size = 0;
		download_size = 0;
	}

	file_info->offset = 0;
	file_info->callback = callback;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	file_info->current_time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
	
	rpc_storage_download(rpc, vmid, download_size, callback_storage_download, file_info);
	sync_status = false;

	return 0;
}

static int cmd_md5(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_storage_md5(bool result, uint32_t _md5[], void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(!result) {
			callback("false", -1);
			return false;
		}

		uint8_t md5[16];
		int pos = 0;
		memcpy(md5, _md5, 16);
		for(int i = 0; i < 16; i++) {
			pos += sprintf(cmd_result + pos, "%02x", md5[i]);
		}

		callback(cmd_result, 0);

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 3)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return -1;
	
	if(!is_uint64(argv[2]))
		return -2;

	uint32_t vmid = parse_uint32(argv[1]);
	uint64_t size = parse_uint64(argv[2]);

	rpc_storage_md5(rpc, vmid, size, callback_storage_md5, callback);
	sync_status = false;

	return 0;
}

static int cmd_status_set(int argc, char** argv, void(callback)(char* result, int exit_status)) {
	bool callback_status_set(bool result, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(result) {
			callback("true", 0);
		} else {
			callback("false", -1);
		}

		return false;
	}

	if(rpc == NULL) {
		return ERROR_RPC_DISCONNECTED;
	}

	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(!is_uint32(argv[1])) {
		return -1;
	}
	
	uint32_t vmid = parse_uint32(argv[1]);
	VMStatus vmstatus;

	if(strcmp(argv[0], "start") == 0)
		vmstatus = VM_STATUS_START;
	else if(strcmp(argv[0], "pause") == 0)
		vmstatus = VM_STATUS_PAUSE;
	else if(strcmp(argv[0], "resume") == 0)
		vmstatus = VM_STATUS_RESUME;
	else if(strcmp(argv[0], "stop") == 0)
		vmstatus = VM_STATUS_STOP;
	else
		return -1;
	
	rpc_status_set(rpc, vmid, vmstatus, callback_status_set, callback);
	sync_status = false;

	return 0;
}


static int cmd_status_get(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_status_get(VMStatus status, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(status == VM_STATUS_STOP)
			callback("stop", 0);
		else if(status == VM_STATUS_PAUSE)
			callback("pause", 0);
		else if(status == VM_STATUS_START)
			callback("start", 0);
		else if(status == VM_STATUS_RESUME)
			callback("resume", 0);
		else if(status == VM_STATUS_INVALID)
			callback("invalid", -1);

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 2)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return -1;
	
	uint32_t vmid = parse_uint32(argv[1]);
	
	rpc_status_get(rpc, vmid, callback_status_get, callback);
	sync_status = false;

	return 0;
}

static int cmd_stdin(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	bool callback_stdin(uint16_t written, void* context) {
		void(*callback)(char* result, int exit_status) = context;

		if(written == 0)
			callback("false", -1);
		else
			callback("true", 0);

		return false;
	}

	if(rpc == NULL)
		return ERROR_RPC_DISCONNECTED;

	if(argc < 4)
		return CMD_STATUS_WRONG_NUMBER;
	
	if(!is_uint32(argv[1]))
		return -1;
	
	if(!is_uint8(argv[2]))
		return -2;
	
	uint32_t vmid = parse_uint32(argv[1]);
	uint8_t thread_id = parse_uint8(argv[2]);
	uint16_t length = strlen(argv[3]);
	argv[3][length++] = '\0';
	
	rpc_stdio(rpc, vmid, thread_id, 0, argv[3], length, callback_stdin, callback);
	sync_status = false;

	return 0;
}

Command commands[] = {
	{
		.name = "exit",
		.desc = "Exit the console",
		.func = cmd_exit
	},
	{
		.name = "quit",
		.desc = "Exit the console",
		.func = cmd_exit
	},
	{
		.name = "help",
		.desc = "Show this message",
		.func = cmd_help
	},
	{
		.name = "echo",
		.desc = "Print variable",
		.args = "[variable: string]*",
		.func = cmd_echo
	},
	{
		.name = "sleep",
		.desc = "Sleep n seconds",
		.args = "time: uint32",
		.func = cmd_sleep
	},
	{
		.name = "connect",
		.desc = "Connect to the host",
		.args = "host: string port: uint16",
		.func = cmd_connect
	},
	{
		.name = "ping",
		.desc = "RPC ping to host",
		.args = "count: uint16",
		.func = cmd_ping
	},
	{
		.name = "create",
		.desc = "Create VM",
		.args = "vmid: uint32, core: uint8 memory: uint32 storage: uint32 "
			"[nic: mac: uint64 port: uint32 ibuf: uint32 obuf: uint32 "
			"iband: uint64 oband: uint64 pool: uint32]* [args: [string]+ ]",
		.func = cmd_vm_create
	},
	{
		.name = "destroy",
		.desc = "Destroy VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_vm_destroy
	},
	{
		.name = "list",
		.desc = "List VM",
		.args = "result: uint32[]",
		.func = cmd_vm_list
	},
	{
		.name = "upload",
		.desc = "Upload file",
		.args = "result: bool, vmid: uint32 path: string size: uint32",
		.func = cmd_upload
	},
	{
		.name = "download",
		.desc = "Download file",
		.args = "result: bool, vmid: uint32 path: string size: uint32",
		.func = cmd_download
	},
	{
		.name = "md5",
		.desc = "MD5 storage",
		.args = "result: hex16 string, vmid: uint32 size: uint64",
		.func = cmd_md5
	},
	{
		.name = "start",
		.desc = "Start VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "pause",
		.desc = "Pause VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "resume",
		.desc = "Resume VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "stop",
		.desc = "Stop VM",
		.args = "result: bool, vmid: uint32",
		.func = cmd_status_set
	},
	{
		.name = "status",
		.desc = "Get VM's status",
		.args = "result: string(\"start\", \"pause\", or \"stop\") vmid: uint32",
		.func = cmd_status_get
	},
	{
		.name = "stdin",
		.desc = "Send Standard Input Message",
		.args = "vmid: uint32 threadid: uint8 message: string",
		.func = cmd_stdin
	},
	{
		.name = NULL
	},
};

size_t cmd_count = sizeof(commands) / sizeof(Command);

int main(int _argc, char** _argv) {
	cmd_init();

	void cmd_callback(char* result, int exit_status) {
		cmd_update_var(result, exit_status);

		printf("%s\n", result);
		fflush(stdout);

		sync_status = true;
	}

	int execute_cmd(char* line, bool is_dump) {
		//if is_dump == true then file cmd
		//   is_dump == false then stdin cmd
		if(is_dump == true)
			printf("%s\n", line);
		
		int exit_status = cmd_exec(line, cmd_callback);

		if(exit_status != 0) {
			if(exit_status == CMD_STATUS_WRONG_NUMBER) {
				printf("wrong number of arguments\n");

				return ERROR_CMD_EXECUTE;
			} else if(exit_status == CMD_STATUS_NOT_FOUND) {
				printf("Can not found command\n");

				return ERROR_CMD_EXECUTE;
			} else if(exit_status == CMD_VARIABLE_NOT_FOUND) {
				printf("Variable not found\n");

				return ERROR_CMD_EXECUTE;
			} else if(exit_status == ERROR_MALLOC_NULL) {
				printf("Can not Malloc\n");

				return ERROR_MALLOC_NULL;
			}else if(exit_status == ERROR_RPC_DISCONNECTED) {
				printf("RPC Disconnected\n");

				return ERROR_RPC_DISCONNECTED;
			} else if(exit_status < 0) {
				printf("Wrong value of argument : %d\n", -exit_status);

				return ERROR_CMD_EXECUTE;
			}
		}

		printf("> ");
		fflush(stdout);

		return exit_status;
	}
	
	List* fd_list = list_create(NULL);

	for(int i = 1; i < _argc; i++) {
		int fd = open(_argv[i], O_RDONLY);
		if(fd != -1) {
			list_add(fd_list, (void*)(int64_t)fd);
		}
	}

	list_add(fd_list, STDIN_FILENO); //fd of stdin
	int get_cmd_line(int fd) {
		static char line[MAX_LINE_SIZE] = {0, };
		char* head;
		int seek = 0;
		static int eod = 0; //end of data
		
		while(eod += read(fd, &line[eod], MAX_LINE_SIZE - eod)) {
			head = line;
			for(; seek < eod; seek++) {
				if(line[seek] == '\n' || line[seek] == '\0') {
					line[seek] = '\0';
					int ret = execute_cmd(head, fd != STDIN_FILENO);

					if(ret == 0) {
						head = &line[seek] + 1;
					} else {
						eod = 0;
						return ret;
					}

					if(sync_status == false) {
						memmove(line, head, eod - (head - line));
						eod -= head - line;
						return 0;
					}
				}
			}

			if(head == line && eod == MAX_LINE_SIZE){ //not found '\n' and head == 0
				printf("Command line is too long %d > %d\n", eod, MAX_LINE_SIZE);
				eod = 0;
				return ERROR_CMD_EXECUTE;
			} else { //not found '\n' and seek != 0
				memmove(line, head, eod - (head - line));
				eod -= head - line;
				seek = eod;
			}

			if(fd == STDIN_FILENO) {
				return 0;
			}
		}

		eod = 0;
		return END_OF_FILE;
	}

	int retval;
	fd_set in_put;
	fd_set temp_in;

	int fd = (int)(int64_t)list_remove_first(fd_list);
	FD_ZERO(&in_put);
	FD_SET(fd, &in_put);

	printf("> ");
	fflush(stdout);

	struct timeval time;
	
	while(is_continue) {
		temp_in = in_put;
		time.tv_sec = 0;
		time.tv_usec = 1000;

		retval = select(fd + 1, &temp_in, 0, 0, &time);//add timer

		if(retval == -1) {
			return -1;
		} else if(retval == 0) {
			//select time over
		} else {
			if(sync_status == true) {
				if(FD_ISSET(fd, &temp_in) != 0) {
					int ret = get_cmd_line(fd);

					if((fd != STDIN_FILENO) && (ret < 0)) {
						FD_CLR(fd, &in_put);
						close(fd);
						fd = (int)(int64_t)list_remove_first(fd_list);
						FD_SET(fd, &in_put);
					}

					if(sync_status == false) {
						FD_CLR(fd, &in_put);
						list_add_at(fd_list, 0, (void*)(int64_t)fd);
						fd = -1;
					}
				}
			}
		}

		if(rpc == NULL)
			continue;

		if(rpc_is_closed(rpc)) {
			rpc = NULL;
			continue;
		}

		rpc_loop(rpc);

		if(sync_status == true && fd == -1) {
			fd = (int)(int64_t)list_remove_first(fd_list);
			FD_SET(fd, &in_put);
		}
	} 
	
	return 0;
}
