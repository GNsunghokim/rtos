; Ref: http://os.phil-opp.com/
; Ref: http://download-mirror.savannah.gnu.org/releases/grub/phcoder/multiboot.pdf

%define STACK_SIZE	256
%define CORE_COUNT	16

global start
global gdtr
global lgdt
global change_cs

[bits 16]
section .entry
	mov	ecx, cr0
	and	ecx, 1
	jnz	start

	mov	ax, 0x1000
	mov	ds, ax
	mov	es, ax

	cli
	; Load GDTR symbol address by real mode addressing
	; cs register is 0x1000 now, so we have to subtract 0x10000
	lgdt	[dword gdtr - 0x10000]

	; Change to protected mode
	mov	eax, cr0
	or	eax, 1				; PE=1
	mov	cr0, eax

	; Far jump to protected mode symbol by linear addressing
	jmp	dword 0x18:protectedmode

[bits 32]
	align	8, db 0

header_start:
	dd	0xe85250d6			; multiboot2 magic
	dd	0				; architecture 0 (i386)
	dd	header_end - header_start	;
	dd	0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))	; checksum

	; Information request
	align	8, db 0
.info_start:
	dw	1
	dw	0
	dd	.info_end - .info_start
	dd	3				; module
	dd	4				; basic meminfo
	dd	5				; bootdev
	dd	6				; mmap
.info_end:

	; Entry address header
.addr_start:
	align	8, db 0
	dw	3
	dw	0
	dd	.addr_end - .addr_start
	dd	0x10000
.addr_end:

	; End of multiboot header
	align	8, db 0
	dw	0				; type
	dw	0				; flags
	dd	8				; size
header_end:

protectedmode:
	mov	ax, 0x20
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	jmp	dword 0x18:start

start:
	mov	[stack], eax
	mov	[stack + 4], ebx

	mov	eax, 0x01
	cpuid
	shrd	ebx, ebx, 24 ;cpuid
	and	ebx, 0x000000ff
	mov	esp, stack_top

repeat:
	cmp	ebx, 0
	je	next

	sub	esp, STACK_SIZE

	sub	ebx, 1
	jmp	repeat

next:
	mov	eax, [stack]
	mov	ebx, [stack + 4]
	push	ebx
	push	eax
	extern	main
	call	main

	jmp	0x08:0x200000   ; Jump to kernel64

.loop:
	hlt
	jmp .loop

locked:				; The lock variable. 1 = locked, 0 = unlocked.
	dd      0

stack:				; Stack position
	dd	0
	dd	0

lgdt:
	lgdt	[eax]
	ret

change_cs:
	jmp	0x18:.newcs
.newcs:
	ret

section .rodata
align 8, db 0

	dw	0x0000			; padding for GDTR

gdtr:
	dw	gdtend - gdt - 1	; GDT size
	dd	gdt ; GDT address

gdt:
	; null descriptor
	dw	0x0000
	dw	0x0000
	db	0x00
	db	0x00
	db	0x00
	db	0x00

	; kernel code segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x9a	; p=1, dpl=0, code segment, execute/read
	db	0xaf	; g=1, d=0, l=1, limit 19:16
	db	0x00	; base 31:24

	; kernel data segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x92	; p=1, dpl=0, data segment, read/write
	db	0xaf	; g=1, d=0, l=1, limit 19:16
	db	0x00	; base 31:24

	; loader code segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x9a	; p=1, dpl=0, code segment, read/write
	db	0xcf	; g=1, d=1, l=0, limit 19:16
	db	0x00	; base 31:24

	; loader data segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x92	; p=1, dpl=0, data segment, read/write
	db	0xcf	; g=1, d=1, l=0, limit 19:16
	db	0x00	; base 31:24
gdtend:

section .bss
align 8

stack_bottom:
	resb	STACK_SIZE * CORE_COUNT	; Maximum 16 cores
stack_top:
