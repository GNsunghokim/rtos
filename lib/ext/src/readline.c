#include <util/ring.h>
#include <readline.h>

extern char* __stdin;
extern volatile size_t __stdin_head;
extern volatile size_t __stdin_tail;
extern volatile size_t __stdin_size;

static char line[READLINE_MAX_LINE_SIZE];
static int line_len;

char* readline() {
	char ch;
	size_t len = ring_read(__stdin, &__stdin_head, __stdin_tail, __stdin_size, &ch, 1);
	while(len > 0) {
		line[line_len++] = ch;
		if(ch == '\0' || ch == '\n' || line_len >= READLINE_MAX_LINE_SIZE) {
			line[line_len - 1] = '\0';
			line_len = 0;
			return line;
		}
		
		len = ring_read(__stdin, &__stdin_head, __stdin_tail, __stdin_size, &ch, 1);
	}
	
	return NULL;
}
