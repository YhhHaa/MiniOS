%include "include/boot.inc"
section loader vstart=LOADER_BASE_ADDR

; -----------gdt and descriptor-----------
GDT_BASE:	dd		0x00000000
			dd		0x00000000

CODE_BASE:	dd		0x0000ffff
			dd		DESC_CODE_HIGH4

DATA_STACK_DESC:    dd		0x0000ffff
					dd		DESC_DATA_HIGH4

VIDEO_DESC:	 dd		0x80000007			; limit=(0xbffff-0xb8000)/4k=0x7
			 dd		DESC_VIDEO_HIGH4	; Current dpi is 0.
									 
GDT_SIZE	equ		$ - GDT_BASE
GDT_LIMIT	equ		GDT_SIZE - 1

times	60	dq	0	; 60 blanks.

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
	
	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR
	mov ecx, 200
	call rd_disk_m_32

	call setup_page
	sgdt [gdt_ptr]	; Store the past gdt address


	; video segment address + 3G
	mov ebx, [gdt_ptr + 2]
	or dword [ebx + 0x18 + 4], 0xc0000000

	; move gdt to 3G+
	add dword [gdt_ptr + 2], 0xc0000000

	; change stack address
	add esp, 0xc0000000

	; cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax

	; cr0
	mov eax, cr0
	or eax, 0x8000_0000
	mov cr0, eax

	; restore new gdt
	lgdt [gdt_ptr]

	; test
	mov byte [gs:160], 'V'

	; ----------- enter kernel ------------------
	call kernel_init
	mov esp, 0xc009_f000 ; Just for convenience.
	jmp KERNEL_ENTRY_POINT

; --------------- load kernel ------------------------------------------
rd_disk_m_32:
	; Backup
	mov esi, eax	; Backup eax: LBA addr of start sector.
	mov di, cx		; Backup cx: number of sectors to be written.

	; 1. setcor count
	mov dx, 0x1f2
	mov al, cl
	out dx, al
	mov eax, esi

	; 2. addr
	mov dx, 0x1f3
	out dx, al

	mov dx, 0x1f4
   	shr eax, 8
	out dx, al

	mov dx, 0x1f5
	shr eax, 8
	out dx, al

	mov dx, 0x1f6
	shr eax, 8
	and al, 0000_1111b
	or al, 1110_0000b
	out dx, al

	; 3. read command
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

	; 4. check disk status
	.not_ready:
		nop
		in al, dx
		and al, 1000_1000b
		cmp al, 0000_1000b
		jnz .not_ready
	
	; 5. read from disk
	mov ax, di ; Number of sectors.
	mov dx, 256
	mul dx		 ; Times to read->ax.
	mov cx, ax
	mov dx, 0x1f0
	.go_on_read:
		in ax, dx
		mov [ebx], ax
		add ebx, 2
		loop .go_on_read
		ret


; --------------- Create Page Directory Table and Page Table ------------
setup_page:
; 1.  Clear the data from 0x10_0000 to 0x10_1000.
	mov ecx, 4096  ; 4K times to loop.
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

; 2. Create Page Directory Entry.
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS	; 0x10_0000.
	add eax, 0x1000
	mov ebx, eax		; The base address of the first Page Table: 0x10_1000(Base+4KB).
	
	; Entries higher than the 768th entry were occupied by kernel space(0xc000_0000-0xffff_ffff).
	or eax, PG_US_U | PG_RW_W | PG_P	; U,W,P attributes: 0x7 or addr: 0x10_1000_=0x10_1007=eax.
	mov [PAGE_DIR_TABLE_POS + 0x000], eax  ; Write attr(0x10_1007) to [0x10_0000](the 0th entry)
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax  ; Write attr(0x10_1007) to [0x10_0c00](the 768th entry)
	
	; Set the last pde(the 1023th entry) to eax=0x10_0007 which pointed to itself.
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax

; 3. Create PTE.
	mov ecx, 256	; 1M of low memory / 4k per page = 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P	; Attr: 0x7
.create_pte:
	mov [ebx+esi*4], edx	; [(0x10_1000)+4k*n]=0x7+4096*n.
	add edx, 4096
	inc esi
	loop .create_pte

; 4. Create the PDE of other Page Tables from 768~1022(1023 pointed to itself).
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254
	mov esi, 769
.create_kernel_pde:
	mov [ebx+esi*4], eax
	inc esi
	add eax, 0x1000		; eax=0x10_2000 + 0x1000*n
	loop .create_kernel_pde
	ret

; -------------- copy segment from kernel.bin ------------
kernel_init:
	xor eax, eax	; Just init.
	xor ebx, ebx	; Address of program header table.
	xor ecx, ecx	; Number of program headers.
	xor edx, edx	; Size of program header.

	mov ebx, [KERNEL_BIN_BASE_ADDR + 28] ; This is where the offset of program header at.
	add ebx, KERNEL_BIN_BASE_ADDR		; Get physical address of program header.

	mov cx, [KERNEL_BIN_BASE_ADDR + 44] ; This is the number of program header.

	mov dx, [KERNEL_BIN_BASE_ADDR + 42]	; This is the size of program header.

	.each_segment:
		cmp byte [ebx + 0], PT_NULL ; If type of program header table was PT_NUll, it was not used.
		je .PTNULL

		push dword [ebx + 16]	; This is p_filesz of program header.

		mov eax, [ebx + 4] ; This is p_offset of program header.
		add eax, KERNEL_BIN_BASE_ADDR ; Get physical address of program header.
		push eax

		push dword [ebx + 8] ; This is p_vaddr of program header.

		call mem_cpy
		add esp, 12
	
	.PTNULL:
		add ebx, edx	; Jump to next program header.
		loop .each_segment
		ret

mem_cpy:
	cld	; Set address direction.
	; Backup ebp.
	push ebp
	mov ebp, esp

	; Backup the loop times of outer function.
	push ecx

	mov edi, [ebp + 8] ; dst
	mov esi, [ebp + 12] ; src
	mov ecx, [ebp + 16] ; size
	rep movsb

	pop ecx
	pop ebp
	ret
