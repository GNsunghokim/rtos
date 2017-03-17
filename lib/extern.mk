# This makefile is for external libraries which need to be configured.
.PHONY: all clean cleanall newlib doc

all: libc.a libm.a libexpat.a libz.a libssl.a libxdr.a libvnic.a

newlib/Makefile:
	cd newlib; CFLAGS="-fno-stack-protector -D__SSE_4_1__ -msse4.1" ./configure --target=x86_64-pc-packetngin --disable-multilib --enable-newlib-elix-level=4

PACKETNGIN_BASE = $(shell pwd)

newlib/x86_64-pc-packetngin/newlib/libc.a: newlib/Makefile
	PATH=$$PATH:$(PACKETNGIN_BASE)/bin make -C newlib

libc.a: newlib/x86_64-pc-packetngin/newlib/libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libc.a .

libm.a: libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libm.a .

newlib: newlib/Makefile
	PATH=$$PATH:$(PACKETNGIN_BASE)/bin make -C newlib
	cp newlib/x86_64-pc-packetngin/newlib/libc.a .

expat/lib/libexpat.a:
	cd expat; ./configure --prefix=$(PACKETNGIN_BASE)/expat CFLAGS="-g -fno-stack-protector -O3 -m64"
	cd expat; make
	cd expat; make install

libexpat.a: expat/lib/libexpat.a
	cp expat/lib/libexpat.a .
	mkdir -p ../include
	cp -rL expat/include/* ../include

zlib/libz.a:
	cd zlib; CFLAGS="-fno-stack-protector -O3" ./configure --static --64
	make -C zlib

libz.a: zlib/libz.a
	cp zlib/libz.a .
	mkdir -p ../include
	cp -rL zlib/*.h ../include/

openssl/libssl.a:
	cd openssl; CFLAGS="-fno-stack-protector -O3" ./Configure packetngin-x86_64 no-threads no-shared no-dso
	PACKETNGIN_BASE=$(PACKETNGIN_BASE) make -C openssl build_libs

libssl.a: openssl/libssl.a
	cp openssl/libssl.a .
	cp openssl/libcrypto.a .
	cp -rL openssl/include/* ../include/

xdr/linux/libxdr.a:
	make -C xdr -f Makefile.unix linux/libxdr.a

libxdr.a: xdr/linux/libxdr.a
	cp xdr/linux/libxdr.a .

libvnic.a: vnic/libvnic.a
	cp vnic/libvnic.a .

doc:
	make -C core

clean:
	rm -f libc.a libm.a libz.a libssl.a libcrypto.a libexpat.a libxdr.a

cleanall: clean
	-make -C newlib distclean
	-make -C zlib distclean
	-make -C openssl clean
	-make -C expat clean
	-make -C xdr -f Makefile.unix realclean
