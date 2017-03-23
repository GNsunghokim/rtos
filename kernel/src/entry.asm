[bits 64]

section .entry

extern main

start:
	; Jump to kernel area
	mov	rax, 0xffffffff80200009
	jmp	rax
	
;0xffffffff80200009
	; Data segment descriptors
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	
	; Stack segment descriptors
	mov	ss, ax
	mov	rsp, 0xffffffff805c0000
	mov	rbp, 0xffffffff805c0000

	call	main
	
loop:
	hlt
	hlt
	jmp	loop

times 512 - ($ - $$) db 0x00
