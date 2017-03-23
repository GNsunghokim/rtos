# PacketNgin SDK Documentation {#core}

# Overview {#overview}
PacketNgin SDK (Software Development Kit) provides basic binary libraries to a 
PacketNgin Network Applications (or Net Apps) and various tools to test and emulate Net Apps.
Most important library for a Net App is PacketNgin Core Library (or just libcore)
which is a firmware for Net Apps and also provides useful utilities to manipulate 
virtualized resources such as global and local memory, threads(or CPU Core), 
network interface controller, storage, ...


# Basic Concept {#basic}

## Virtual Machine {#vm}
PacketNgin O/S allocates Real-Time Virtual Machine(RTVM or just VM) to user. PacketNgin VM
consists of vCPU, vMemory, vStorage and vNIC just like other hypervisors such as Xen or KVM.
But PacketNgin VM supports real-time facility to execute real-time Net Apps such as
VoIP router or video conferencing gateway. PacketNgin O/S allocates physical resources to a VM
excluisively. For example PacketNgin O/S allocates few CPU Cores to a VM and it never time-share
the resource with other VMs. PacketNgin O/S does not support memory swapping technology for it is not 
suitable for real-time applications.

PacketNgin VM is difference from other hypervisor's one on the other side. PacketNgin VM
cannot execute general purpose O/Ses like Linux or Windows. PacketNgin SDK provides special purpose
firmware binary named PacketNgin Core Library to user to program the Net Apps. 
It is not a exceptional case for various network processors(NP) such as Octeon NP from CaviumNetworks,
Tile NP from Tilera provides firmware binaries to user to program the Net Apps.

PacketNgin Core Library supports basic APIs to manage CPU, Memory and NIC of PacketNgin VM.


## CPU and Threads {#cpu}
PacketNgin O/S allocates one or more CPU cores to a Net App. Net App manages CPU Cores as threads. 
So one thread maps to one CPU core exactly. The first thread receives thread id as 0, 
and other threads receives thread id 1 to n sequencially.

Threads can communicate with other threads with global memory area. Every threads in a Net App share 
global memory area. Natually there is concurrency issues when using global memory area.

One thread allocate local memory area which is exclusively allocated to one thread only.
It's convenient to consider local memory area as thread local memory. No other thread but associated
thread can access it's own local memory area.

@see thread.h
@see status.h
@see lock.h


## Global and shared memory area {#gmalloc}
Global memory area is sharable memory area between threads in a Net App. Any threads
in a Net App can read or write global memory area. Shared memory area is a specially
allocated area from global memory area to share the pointer (address of memory) between threads.
PacketNgin O/S does not allocate global memory area with predefined addresses. So a thread
has no knowedge of global memory address to communicates with other threads.
User can allocate global memory address and register the pointer as a shared memory area.
When one thread register a chunk of global memory area as a shared memory, 
other threads can communicates via shared memory.

@see gmalloc.h
@see shared.h


## Local memory area {#malloc}
Local memory area is explicitly allocated memory area to a thread. So other thread cannot
read or write local memory area. There is noway to access other thread's local memory area.
So it's reasonable to consider local memory area as thread local memory.

@see malloc.h


## Network Interface Controller {#nic}
PacketNgin O/S allocates number of vNIC(virtualized Network Interface Controller)s to a 
PacketNgin VM. Every vNIC has its own MAC address which is randomly generated or allocated 
from centralized management system. PacketNgin O/S also allocates some quantity of bandwidth
to a vNIC exclusively. Net App can recevie OSI(Open Systems Interconneciton) model level 2
Packet (just Ethernet) directly from vNIC without passing network protocol stack of PacketNgin O/S. 
Which is very different approach from general purpose O/S like Linux, for general purpose O/S
encapsulates or decapsulates OSI level 2 ~ 4 packet headers and it just pass payload data to
user applications. 

PacketNgin O/S is a specialized network O/S to execute Net Apps such as firewalls, 
load balancers, or protocol converting gateways. So it's natural to pass packet header payload
to Net Apps without passing through PacketNgin O/S's network protocol stack.

@see net/ni.h
@see net/packet.h
@see net/ether.h
@see net/arp.h
@see net/icmp.h
@see net/ip.h
@see net/udp.h
@see net/tcp.h
@see net/crc.h
@see net/checksum.h
@see net/md5.h


## Utility functions {#util}
Core library support basic utilities such as read line from buffer, data structure, 
command parsing and event processing engine to build a Net App.

@see readline.h
@see util/list.h
@see util/vector.h
@see util/fifo.h
@see util/ring.h
@see util/map.h
@see util/cmd.h
@see util/event.h


# Other libraries {#libs}
PacketNgin SDK supports various useful libraries. Here is the list of the libraries,
ported to PacketNgin RTOS.

  * Newlib - Standard C library (https://sourceware.org/newlib/)
  * OpenSSL - A cryptography library (https://www.openssl.org/)
  * LwIP - Light weight IP protocol stack (http://savannah.nongnu.org/projects/lwip/)
  * zlib - A compression library (http://www.zlib.net/)

We need more hands to port more libraries to PacketNgin RTOS.


# Tools {#tools}
PacketNgin SDK provides various tools to compile, test Net Apps and 
to emulate virtualized network in a PC.

## Compile and Link
libpacketngin is a combined library of libcore for firmware, newlib for standard C,
and TLSF for real-time memory allocation. User can use GCC for compile and link with
special options.

  * -m64 for PacketNgin RTOS is running on x86_64 only
  * -ffreestanding for Net App doesnot use GCC's default C library but Newlib
  * -fno-stack-protector for PacketNgin RTOS protects heap and stack memory using 
    virtual memory not stack pointer

## Test
Net App can be tested using QEMU or VirtualBox in developer's PC. Just launch 
PacketNgin RTOS on QEMU or VirtualBox then upload Net App binary image to the O/S
and execute it using PacketNgin Console. Which can be used to manipulate 
PacketNgin RTOS from remote.

User can also use PacketNgin Network Emulator to make a virtualized network 
on a PC and test Net Apps. Network emulation is fundamental tool to develop Net Apps.
For most of Net Apps does not act as a end-point host but middle-box node.
So user need to launch various end-point hosts using virtualization technology,
such as VirtualBox or Xen and need to emulate network itself on a PC.


# Supported Platforms {#platform}
PacketNgin RTOS is newbie in O/S world so it does not support various H/W devices. 
Here is the list of devices which the O/S supports.

## CPUs
  * Intel Core i5
  * Intel Core i7
  * QEMU and VirtualBox with x86_64 CPU virtualization

## Network Devices
  * RealTek 8111 (not fully supported yet)
  * NetFPGA 1G-CML
  * Virt I/O


# Contact point {#contact}

<table border="0">
	<tr>
		<td> PacketNgin OSS Community </td> 
		<td> http://packetngin.org </td>
	</tr>
	<tr>
		<td> PacketNgin Commercial Support </td>
		<td> http://packetngin.com </td>
	</tr>
	<tr>
		<td> PacketNgin Help Desk </td>
		<td> packetngin at gurum dot cc </td>
	</tr>
</table>
