.PHONY: all clean

CFLAGS := -I ../../lib/ext/include -Wall -Werror -std=gnu99

all: console

console: src/console.c
	gcc $(CFLAGS) -Wno-unused-variable -o $@ $^ -lcurl -DLINUX ../../lib/ext/bin/linux/libcore.a
	cp -f $@ ../../bin

clean:
	rm -f console
