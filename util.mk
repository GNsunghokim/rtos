# Utility Makefile for PacketNgin RTOS
.PHONY: run stop ver deploy sdk gdb dis help

all: run

# Default running option is QEMU
ifndef option
option	:= qemu
endif

# QEMU related options
HDD	:= -hda system.img
USB	:= -drive if=none,id=usbstick,file=./system.img -usb -device usb-ehci,id=ehci -device usb-storage,bus=ehci.0,drive=usbstick
VIRTIO	:= -drive file=./system.img,if=virtio
NIC	:= virtio
QEMU	:= qemu-system-x86_64 $(shell bin/qemu-params) -m 1024 -M pc -smp 8 -d cpu_reset -net nic,model=$(NIC) -net tap,script=bin/qemu-ifup -net nic,model=$(NIC) -net tap,script=bin/qemu-ifup $(VIRTIO) --no-shutdown --no-reboot

run: system.img
# Run by QEMU
ifeq ($(option),qemu)
	sudo $(QEMU) -monitor stdio
endif
ifeq ($(option),cli)
	sudo $(QEMU) -curses
endif
ifeq ($(option),vnc)
	sudo $(QEMU) -monitor stdio -vnc :0
endif
ifeq ($(option),debug)
	sudo $(QEMU) -monitor stdio -S -s
endif

# Run by VirtualBox
ifeq ($(option),vb)
	$(eval UUID = $(shell VBoxManage showhdinfo system.vdi | grep UUID | awk '{print $$2}' | head -n1))
	rm -f system.vdi
	VBoxManage convertfromraw system.img system.vdi --format VDI --uuid $(UUID)
	VBoxManage startvm PacketNgin
endif

stop:
# Stop QEMU
ifeq ($(option),qemu)
	sudo killall -v -9 qemu-system-x86_64
endif
# Stop VirtualBox
ifeq ($(option),vb)
	VBoxManage controvlm PacketNgin poweroff
endif

ver:
	@echo "Current PacketNgin RTOS version"
	@echo $(shell bin/ver.sh)

deploy: system.img
	@echo "Deploy PacketNgin RTOS image to USB"
	bin/deploy

SDK := packetngin_sdk-$(shell bin/ver.sh)

sdk: loader/loader.bin kernel.bin initrd.img system.img
	@echo "* Create PacketNgin SDK(Software Development Kit)"
	@echo "* Make PacketNgin SDK directory: $(SDK)"
	mkdir -p $(SDK)

	@echo "* Copy SDK examples"
	cp -r examples $(SDK)/examples

	@echo "* Copy system images"
	mkdir -p $(SDK)/bin
	cp $^ $(SDK)/bin

	@echo "* Copy library headers"
	mkdir -p $(SDK)/include
	cp -r include/* $(SDK)/include

	@echo "* Copy library: core"
	mkdir -p $(SDK)/lib
	cp lib/libpacketngin.a $(SDK)/lib

	@echo "* Copy library: OpenSSL"
	cp lib/libcrypto.a lib/libssl.a $(SDK)/lib

	@echo "* Copy library: LwIP"
	cp lib/liblwip.a $(SDK)/lib

	@echo "* Copy library: zlib"
	cp lib/libz.a $(SDK)/lib

	@echo "* Copy library: expat"
	cp lib/libexpat.a $(SDK)/lib

	@echo "* Copy library: JSMN"
	cp lib/libjsmn.a $(SDK)/lib

	@echo "* Copy utilities"
	cp -rL bin/* $(SDK)/bin/

	@echo "* Archiving to $(SDK).tgz"
	tar cfz $(SDK).tgz $(SDK)

gdb:
	@echo "Run GDB session connected to PacketNgin RTOS"
	# target remote localhost:1234
	# set architecture i386:x86-64
	# file kernel/kernel.elf
	gdb --eval-command="target remote localhost:1234; set architecture i386:x86-64; file kernel/kernel.elf"

dis: kernel/build/kernel.elf
	@echo "Dissable PacketNgin kernel image"
	objdump -d kernel/build/kernel.elf > kernel.dis && vi kernel.dis

clean: Build.make
	@${MAKE} --no-print-directory -C . -f Build.make clean

help:
	@echo "Usage: make [target] [option=name]"
	@echo ""
	@echo "TARGETS:"
	@echo "	all (default)		- run"
	@echo "	run [option]		- Run PacketNgin RTOS by emulator"
	@echo "	stop			- Stop PacketNgin RTOS"
	@echo "	test			- Test & run PacketNgin RTOS"
	@echo "	ver			- Echo PacketNgin RTOS version"
	@echo "	deploy			- Deploy PacketNgin RTOS image to USB"
	@echo "	sdk			- Create PacketNgin SDK(Software Development Kit)"
	@echo "	gdb			- Run GDB session connected to PacketNgin RTOS"
	@echo "	dis			- Disassemble PacketNgin kernel image"
	@echo ""
	@echo "OPTION:"
	@echo "	qemu (default)		- Run by QEMU"
	@echo "	cli			- Run CLI mode (QEMU)"
	@echo "	vnc			- Run VNC mode (QEMU)"
	@echo "	debug			- Run GDB mode (QEMU)"
	@echo "	test			- Execute runtime-tests (QEMU)"
	@echo "	vb			- Run by VirtualBox"
	@echo ""
