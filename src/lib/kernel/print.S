%include "include/boot.inc"


[bits 32]
section .data
put_int_buffer dq 0

section .text
; ---------- put_char ---------------------------------------------
; Function: Put one character in stack into at where text cursor.
; -----------------------------------------------------------------
global put_char
put_char:
    ; 1. Backup 8 4B registers.
    pushad

    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ;2.Get position of text cursor.
    mov dx, 0x03d4 ; Index register.
    mov al, 0x0e ; Index of Cursor location register high 8.
    out dx, al
    mov dx, 0x03d5 ; Data of CUrsor location register.
    in al, dx
    mov ah, al

    mov dx, 0x03d4 ; Index register.
    mov al, 0x0f ; Index of Cursor location register low 8.
    out dx, al
    mov dx, 0x03d5
    in al, dx

    mov bx, ax ; Store location into bx.

    ;3.Get the strings.
    mov ecx, [esp + 36] ; 4*8+4 Get strings.
    cmp cl, 0xd ; carriage return=0xd
    jz .is_carriage_return
    cmp cl, 0xa ; line feed=0xa
    jz .is_line_feed
    cmp cl, 0x8 ; backspace=8
    jz .is_backspace
    jmp .put_other


    .is_backspace:
        dec bx
        shl bx, 1
        mov byte [gs:bx], 0x20 ; Fill it with space.
        inc bx
        mov byte [gs:bx], 0x07 ; Font attributes.
        
        shr bx, 1

        jmp .set_cursor

    .put_other:
        shl bx, 1
        mov byte [gs:bx], cl
        inc bx
        mov byte [gs:bx], 0x07
        shr bx, 1
        inc bx
        cmp bx, 2000
        jl .set_cursor

    .is_line_feed:
    .is_carriage_return:
        xor dx, dx
        mov ax, bx
        mov si, 80

        div si
        sub bx, dx

    .is_carriage_return_end:
        add bx, 80
        cmp bx, 2000
    .is_line_feed_end:
        jl .set_cursor

    .roll_screen:
        ; Move up one row.
        cld
        mov ecx, 960 ; [(2000-80)*2]/4=960
        mov esi, 0xc00b80a0
        mov edi, 0xc00b8000
        rep movsd

        ; Fill the last row with blank.
        mov ebx, 3840 ; The first character address of the last row.
        mov ecx, 80
        .cls:
            mov word [gs:ebx], 0x0720
            add ebx, 2
            loop .cls
            mov bx, 1920

    .set_cursor:
        ; High 8 register.
        mov dx, 0x03d4
        mov al, 0x0e
        out dx, al
        mov dx, 0x03d5
        mov al, bh
        out dx, al

        ; Low 8 register.
        mov dx, 0x03d4
        mov al, 0x0f
        out dx, al
        mov dx, 0x03d5
        mov al, bl
        out dx, al
        .put_char_down:
            popad
            ret

global put_int
put_int:
    pushad
    mov ebp, esp
    mov eax, [ebp + 4 * 9]
    mov edx, eax
    mov edi, 7  ; offset in put_int_buffer.
    mov ecx, 8  ; number of 32bits in hexadecimal notation.
    mov ebx, put_int_buffer

    .16based_4bits:
        and edx, 0x0000_000f ; get low 4 bits.
        cmp edx, 9 ; A-F jump to .is_A2F.
        jg .is_A2F

        add edx, '0' ; get ascii.
        jmp .store
    
    .is_A2F:
        sub edx, 10
        add edx, 'A'
    
    .store:
        mov [ebx + edi], dl
        dec edi
        shr eax, 4
        mov edx, eax
        loop .16based_4bits
    
    .ready_to_print:
        ; Delete zero at high bits.
        inc edi ; current edi is (-1)0xffff_ffff, add 1 to edi to make it be 0.
    .skip_prefix_0:
        cmp edi, 8
        je .full0
    
    .go_on_skip:
        mov cl, [put_int_buffer + edi]
        inc edi
        cmp cl, '0'
        je .skip_prefix_0

        dec edi ; until it is not zero.
        jmp .put_each_num
    
    .full0:
        mov cl, '0'
    
    .put_each_num:
        push ecx ; print character.
        call put_char

        add esp, 4 ; clear parameter in stack.
        inc edi ; update data in ecx.
        mov cl, [put_int_buffer + edi]

        cmp edi, 8 ; until print all characters.
        jl .put_each_num

        popad ; exit.
        ret

; ---------- put_str ---------------------------------------------
; Function: print strings with put_char function.
; -----------------------------------------------------------------
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp + 12]

    .goon:
        mov cl, [ebx]
        cmp cl, 0
        jz .str_over

        push ecx
        call put_char
        add esp, 4
        inc ebx
        jmp .goon

    .str_over:
        pop ecx
        pop ebx
        ret
