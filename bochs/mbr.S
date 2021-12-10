;Main boot record;

;Include the configuration.
%include "include/boot.inc"

SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov ax, 0xb800 ;Text mode.
	mov gs, ax

	;Clear screen: int 0x10.
	;	- function number:ah-0x06
	;	- lines: al-0x00(all)
	;	- attr: bh-0x07
	;	- left top: (cl, ch)=(0, 0)
	;	- right bottom: (dl, dh)=(24, 79)
	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0
	mov dx, 0x184f
	int 0x10

	;Screen print.
	mov byte [gs:0x00], '1'
	mov byte [gs:0x01], 0xa4

	mov byte [gs:0x02], ' '
	mov byte [gs:0x03], 0xa4

	mov byte [gs:0x04], 'M'
	mov byte [gs:0x05], 0xa4

	mov byte [gs:0x06], 'B'
	mov byte [gs:0x07], 0xa4

	mov byte [gs:0x08], 'R'
	mov byte [gs:0x09], 0xa4

	; Loader program.
	mov eax, LOADER_START_SECTOR ; LBA addr of start sector: 0x2.
	mov bx, LOADER_BASE_ADDR	; Memory addr of loader: 0x900.
	mov cx, 4					; Number of sectors to be written: 4.
	call rd_disk_m_16			; Read a sector of program.

	jmp LOADER_BASE_ADDR + 0x300


; Read n sectors of disk.
rd_disk_m_16:
; eax: LBA sector number.
; bx: Memory addr of loader.
; cx: NUmber of sectors to be written.
	
	; Backup
	mov esi, eax	; Backup eax: LBA addr of start sector.
	mov di, cx		; Backup cx: number of sectors to be written.

	; Read and write disk.
	; 1. Set number of sectors to be read.
	mov dx, 0x1f2	; Sector count register at 0x1f2 for primary channel.
	mov al, cl		; Number of sectors to be written: 1.
	out dx, al		; Output the number of sectors to dx[0x1f2]

	mov eax, esi	; Recover eax.

	; 2. Output LBA addr to 0x1f3~0x1f6
	mov dx, 0x1f3	; 7~0 bits(0x02) to 0x1f3
	out dx, al

	mov cl, 8		; 15~8 bits(0x00) to 0x1f4
	shr eax, cl		; 0x02>>8=0x00
	mov dx, 0x1f4
	out dx, al

	shr eax, cl		; 23~9 bits(0x00) to 0x1f5
	mov dx, 0x1f5
	out dx, al

	shr eax, cl		; eax=0x00.
	and al, 0x0f	; 24~27 bits(0x00) to device register 0~3 bits.
	or al, 0xe0		; Device register 4~7: al=1110 0000.
	mov dx, 0x1f6
	out dx, al		; Device register=1110 0000.

	; 3. Output read command to 0x1f7-0x20.
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

	; 4. Detect status of disk.
  .not_ready:
 	nop				; Sleep a while.
	in al, dx		; Input 0x1f7 data to al.
	and al, 0x88	; Get 4: ready and 7: busy bit.
	cmp al, 0x08	; Compare dl with 00001000.
	jnz .not_ready	; Wait until ready.

	;5. Get data from 0x1f0.
	mov ax, di
	mov dx, 256
	mul dx			; Times of read operation: ax = dx * ax = 256 * 4 = 1024.
	mov cx, ax		; Backup cx=ax=1024.
	mov dx, 0x1f0
  .go_on_read:
	in ax, dx		; Read from 0x1f0.
	mov [bx], ax	; Write to [bx]
	add bx, 2
	loop .go_on_read
	ret
	
		
  ;Define the final data.
  times 510-($-$$) db 0
  db 0x55, 0xaa
