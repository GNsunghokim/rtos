#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <net/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define LENGTH		59
#define EX_BUF_MAZ	48 

static void extract_token(int write_fd, char* str);

static void	packet_dump_func(void** state) {
	int write_fd = open("test_packet.txt", O_CREAT | O_WRONLY, 0755);
	int stdout_write_fd = NULL;
	int read_fd = open("test_packet.txt", O_CREAT | O_RDONLY, 0755);

	char comp_ch[128];
	memset(comp_ch, 0, 128);

	// Changing stdout to write_fd.
	dup2(1, stdout_write_fd);
	dup2(write_fd, 1);

	// Creating dummy packet 
	Packet* packet = malloc(sizeof(Packet) + LENGTH);
	packet->start = 0;
	packet->end = LENGTH;

	for(int i = 0; i < LENGTH; i++)
		*((packet->buffer) + i) = i + 10;

	// Printing dummy packet
	packet_dump(packet);

	// Change stdout_fd to original fd
	dup2(stdout_write_fd, 1);

	int index = 0;

	for(int length = 1; length < LENGTH; length++) {
		lseek(read_fd, 0, SEEK_SET);
		
		while(index != LENGTH) {
			extract_token(read_fd, comp_ch);

			// Extracting dummy packet by 16 bytes
			char* ex_buf = (char*)malloc(sizeof(char) * EX_BUF_MAZ);

			for(size_t i = 0; i < (strlen(comp_ch) + 1) / 3; i++) {
				sprintf(ex_buf + i * 3, "%02x", *((packet->buffer) + index++));
				if((i + 1) % 16 != 0)
					sprintf(ex_buf + i * 3 + 2, " ");
				else {
					break;
				}
			}
			
			// Comparing rtn of packet_dump func and dummy packet by line
			assert_string_equal(comp_ch, ex_buf);
			free(ex_buf);
		}
	}
	free(packet);
	close(write_fd);
	close(stdout_write_fd);
	close(read_fd);
}

/**
 * extract_token : This function extracts a single line of string from write_fd.
 */
static void extract_token(int write_fd, char* str) {
	int len = 0;
	memset(str, 0, 128);
	while(1) {
		read(write_fd, str + len, 1);
		if(str[len] == EOF || str[len] == '\n')
			break;
		len++;
	}
	str[len] = '\0';
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(packet_dump_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
