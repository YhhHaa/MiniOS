%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR


; -----------gdt and descriptor-----------
GDT_BASE:			dd		0x00000000
					dd		0x00000000

CODE_BASE:			dd		0x0000ffff
					dd		DESC_CODE_HIGH4

DATA_STACK_DESC:	dd		0x0000ffff
					dd		DESC_DATA_HIGH4

VIDEO_DESC:			dd		0x80000007			; limit=(0xbffff-0xb8000)/4k=0x7
					dd		DESC_VIDEO_HIGH4	; Current dpi is 0.

GDT_SIZE			equ		$ - GDT_BASE
GDT_LIMIT			equ		GDT_SIZE - 1

times	60	dq	0	; 60 blanks.

SELECTOR_CODE 		equ		(0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA		equ		(0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO		equ		(0x0003<<3) + TI_GDT + RPL0

; detect memory. (ip->64*8+0x900=0x200+0x900=0xb00)
total_mem_bytes dd 0

; -----------------gdt point---------------
gdt_ptr     dw      GDT_LIMIT
            dd      GDT_BASE


; total_mem_bytes4+gdt_ptr6+ards_buff244+ards_nr2, total: 256B.
ards_buf times 244 db 0
ards_nr dw 0

; ip=0x300
loader_start:
    ; int 0x15 eax=0xe820 edx=0x534d4150('SMAP')
	xor ebx, ebx			; ebx=0 when first call.
	mov edx, 0x534d4150		; 'SMAP'.
	mov di, ards_buf		; ards buffer.
.e820_mem_get_loop:
	mov eax, 0xe820			; Function.
	mov ecx, 20				; Size.
	int 0x15
	jc .e820_failed_so_try_e801	; If CF==1.
	
	add di, cx				; New ards address(+20).
	inc word [ards_nr]		; Record number of ards.
	cmp ebx, 0				; If ebx==0->zf=0.
	jnz .e820_mem_get_loop	; If zf!=0.
	
	; Find the largest ards.
	mov cx, [ards_nr]
	mov ebx, ards_buf	
	xor edx, edx			; edx is the max memory size(clear before use).
.find_max_mem_area:
	mov eax, [ebx]			; Get base_add_low(4B).
	add eax, [ebx+8]		; length_low(4B) + base_add_low.
	add ebx, 20				; Point to next ards.
	cmp edx, eax			; Max size - (length_low + base_add_low)
	; Bubble sort.
	jge .next_ards			; If edx>=eax.
	mov edx, eax
.next_ards:
	loop .find_max_mem_area	; Loop cx times.
	jmp .mem_get_ok

	; -----------------int 0x15 ax=0xe801--------------
.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_so_try88

	; 1. Low 15 mb memory.
	mov cx, 0x400			; Unit 1KB.
	mul cx					; Mul ax->dx(high):ax(low)
	shl edx, 16
	and eax, 0x0000ffff
	or edx, eax				; edx+eax->edx

	add edx, 0x100000		; +1MB.

	mov esi, edx			; Backup low 15 MB.
	
	; 2. Hight 16MB memory.
	xor eax, eax			; Clear eax->0.
	mov ax, bx
	mov ecx, 0x10000		; Unit 64KB.
	mul ecx					; Mul eax->edx:eax
	add	esi, eax			; Only use low 32 bits because max size of 4GB.
	mov edx, esi			; Store.
	jmp .mem_get_ok

	; ------------int 0x15 ax=0x88-------------------------
.e801_failed_so_try88:
	mov ah, 0x88
	int 0x15
	jc .error_hlt

	and eax, 0x0000ffff
	mov cx, 0x400
	mul cx					; Mul cx*ax->dx:ax
	shl edx, 16
	or edx, eax
	add edx, 0x100000		; +1MB.

.mem_get_ok:
	mov [total_mem_bytes], edx

	; Prepare sth for protection motion.
	; 1. Open A20.
	; 2. Load gdt.
	; 3. Set pe of cr0 1.
	; ------------- open a20 ---------------
	in al, 0x92
	or al, 0000_0010B
	out 0x92, al
	; ------------- load gdt --------------
	lgdt [gdt_ptr]
	; ------------- set pe of cr0 1 -------
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp dword SELECTOR_CODE:p_mode_start	; clear pipeline because 16 and 32bits.

.error_hlt:
	hlt
	

[bits 32]
p_mode_start:
	; init segment register.
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax

	mov esp, LOADER_STACK_TOP

	mov ax, SELECTOR_VIDEO
	mov gs, ax

	mov byte [gs:160], 'P'

	jmp $
