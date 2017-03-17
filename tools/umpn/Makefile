.PHONY: all clean

SDK = ../../
NETAPP =  $(SDK)/examples/echo/src/main.c
CFLAGS = -I $(SDK)/include -I ./include -I ../../lib/TLSF/src -Werror -std=gnu99 -Wall -Dmain=net_app_main
LIBS = ../../lib/libext.a

all: packetngin

run: all
	sudo ./packetngin

packetngin: src/main.c src/tun.c src/ne.c src/ni.c $(NETAPP)
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f packetngin

create_bridge:
	sh ./src/packetngin-ifup

delete_bridge:
	sh ./src/packetngin-ifdown
