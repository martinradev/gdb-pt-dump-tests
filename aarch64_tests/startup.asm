.extern stack_top
.globl _start
_start:
	ldr x30, =stack_top
	mov sp, x30
	bl entry
hang:
	b hang
