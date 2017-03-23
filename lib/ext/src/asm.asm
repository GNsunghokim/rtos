[BITS 64]

SECTION .text

global cmpxchg

cmpxchg:
	mov		rax, rsi
	lock cmpxchg	byte[rdi], dl
	ret

