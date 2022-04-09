[org 0x7c00]

kernel_offset equ 0x1000

start:
	mov bp, 0x8000
	mov sp, bp 

	mov bx, kernel_offset 
	mov dh, 10
	call load_test

	cli
	lgdt [gdt_descriptor]

	mov eax, cr0 
	or eax, 0x1 
	mov cr0, eax 

	jmp CODE_SEG:init

load_test:
	pusha 
	push dx		; number of sectors (input parameter)

	mov ah, 0x02 	; read function 
	mov al, dh 	; number of sectors
	mov dl, 0x00 	; drive number
	mov dh, 0x00 	; head number
	mov ch, 0x00 	; cylinder number  
	mov cl, 0x02 	; sector number 

	; read data to [es:bx] 
	int 0x13
	jc error 	; carry bit is set -> error

	pop dx 
	cmp al, dh 	; read correct number of sectors
	jne error 

	popa 
	ret 

error:
	mov bl, 0xff
	jmp error

[bits 32]
init:
	mov ax, DATA_SEG 
	mov ds, ax 
	mov ss, ax 
	mov es, ax 
	mov fs, ax 
	mov gs, ax 

	mov ebp, 0x80000
	mov esp, ebp 

	call enable_32_bit_paging_no_pae

mov eax, kernel_offset
jmp eax

enable_32_bit_paging_no_pae:
	pusha
	; enable 4 mib pages
	mov eax, 0x10
	mov cr4, eax
	; pde
	mov ebx, 0x10000
	push ebx
	lea ecx, [ebx + 0x1000]
	mov ecx, 0x83
	mov [ebx], ecx
	; set new page table
	pop ebx
	mov cr3, ebx
	; enable paging
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	popa
	ret

gdt_start:

gdt_null:
	dd 0x0
	dd 0x0		; 8-byte

gdt_code:
	dw 0xffff	; limit
	dw 0x0		; base 
	db 0x0		; base
	db 10011010b 	; P, DPL, S, Type flags
	db 11001111b 	; G, D/B, L, AVL flags, limit 
	db 0x0 		; base 

gdt_data:
	dw 0xffff
	dw 0x0 
	db 0x0 
	db 10010010b 	; Type flag is diff
	db 11001111b 
	db 0x0 

gdt_end:

gdt_descriptor:
	; size (16-bit) + addr (32-bit)
	dw gdt_end - gdt_start + 1
	dd gdt_start 

; some useful information
CODE_SEG equ gdt_code - gdt_start 
DATA_SEG equ gdt_data - gdt_start

times 510 - ($ - $$) db 0
db 0x55, 0xaa

