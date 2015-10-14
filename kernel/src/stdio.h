#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>
#include <stdbool.h>

extern char __stdin[];
extern volatile size_t __stdin_head;
extern volatile size_t __stdin_tail;
extern size_t __stdin_size;

extern char __stdout[];
extern volatile size_t __stdout_head;
extern volatile size_t __stdout_tail;
extern size_t __stdout_size;

extern char __stderr[];
extern volatile size_t __stderr_head;
extern volatile size_t __stderr_tail;
extern size_t __stderr_size;

void stdio_init();
void stdio_init2(void* buf, size_t size);
void stdio_dump(int coreno, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size);

int stdio_putchar(const char ch);
void stdio_scancode(int code);
int stdio_getchar();

#endif /* __STDIO_H__ */
