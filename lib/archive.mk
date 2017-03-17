ifndef $(config)
	config=debug
endif

.PHOYNY = clean

all: libcore.a

ifeq ($(config),debug)
libcore.a: libtlsf.a libext.a libc.a libm.a libexpat.a
endif
ifeq ($(config),release)
libcore.a: libtlsf.a libext.a libc.a libm.a libexpat.a
endif
ifeq ($(config),linux)
libcore.a: libtlsf.a libext.a
endif
	echo "Archiving libraries $^ for $(config)"
	$(foreach lib, $^, $(shell ar x $(lib)))
	ar rcs $@ *.o
	rm -rf *.o

clean:
	rm -rf *.a


