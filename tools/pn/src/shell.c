#include <util/cmd.h>
#include "stdio.h"
#include "string.h"
#include "driver/charin.h"
#include "driver/charout.h"
#include "version.h"
#include "device.h"
#include "shell.h"

static bool cmd_sync;

static void history_erase() {
	if(!cmd_history.using())
		// Nothing to be erased
		return;

	/*
	 *if(cmd_history.index >= cmd_history.count() - 1)
	 *        // No more entity to be erased
	 *        return;
	 *        
	 */
	int len = strlen(cmd_history.get_current());
	for(int i = 0; i < len; i++)
		putchar('\b');
}

static void cmd_callback(char* result, int exit_status) {
	cmd_update_var(result, exit_status);
	cmd_sync = false;
}

void shell_callback() {
	static char cmd[CMD_SIZE];
	static int cmd_idx = 0;
	extern Device* device_stdout;
	char* line;

	if(cmd_sync)
		return;

	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				cmd[cmd_idx] = '\0';

				int exit_status = cmd_exec(cmd, cmd_callback);
				if(exit_status != 0) {
					if(exit_status == CMD_STATUS_WRONG_NUMBER) {
						printf("Wrong number of arguments\n");
					} else if(exit_status == CMD_STATUS_NOT_FOUND) {
						printf("Command not found: %s\n", cmd);
					} else if(exit_status == CMD_VARIABLE_NOT_FOUND) {
						printf("Variable not found\n");
					} else if(exit_status < 0) {
						printf("Wrong value of argument: %d\n", -exit_status);
					}
				}

				printf("# ");
				cmd_idx = 0;
				cmd_history.reset();
				break;
			default:
				if(cmd_idx < CMD_SIZE - 1) {
					cmd[cmd_idx++] = ch;
				}
		}
		if(cmd_sync)
			break;

		ch = stdio_getchar();
	}
}

int shell_init() {
	extern Device* device_stdin;
	((CharIn*)device_stdin->driver)->set_callback(device_stdin->id, shell_callback);
	cmd_sync = false;
	cmd_init();

	return 0;
}

void shell_sync() {
	cmd_sync = true;
}
