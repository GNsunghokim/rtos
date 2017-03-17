.PHONY: all clean

all: pnkc/pnkc smap/smap

pnkc/pnkc:
	make -C pnkc

smap/smap:
	make -C smap

# TODO: compile only at config=linux
console:
	make -C console

umpn:
	make -C umpn

clean:
	make -C pnkc clean
	make -C smap clean
	make -C console clean
	make -C umpn clean
