[bits 32]
; 若已经压入错误码, 不做操作. 未压入则保持格式统一手动压入0
%define ERROR_CODE nop
%define ZERO push 0

; 申明外部汇编函数
extern put_str;
extern idt_table;

section .data
    global intr_entry_table
    intr_entry_table:
        %macro VECTOR 2

        section .text
            intr%1entry:
                %2 ; 压入错误码

                ; 保存上下文环境
                push ds
                push es
                push fs
                push gs
                pushad

                ; 发送中断结束命令
                mov al, 0x20 ; 写入状态控制字OCW2
                out 0xa0, al ; 向从片发送
                out 0x20, al ; 向主片发送

                push %1 ; 压入中断向量号
                call [idt_table + %1 * 4] ; 调用C数组中的中断处理函数
                jmp intr_exit
        
        section .data
            dd intr%1entry

        %endmacro

section .text
global intr_exit
intr_exit:
    ; 恢复上下文环境
    add esp, 4 ; 跳过中断向量号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4 ; 跳过error_code
    iretd

VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE
VECTOR 0x09, ZERO
VECTOR 0x0a, ERROR_CODE
VECTOR 0x0b, ERROR_CODE
VECTOR 0x0c, ERROR_CODE
VECTOR 0x0d, ERROR_CODE
VECTOR 0x0e, ERROR_CODE
VECTOR 0x0f, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO
VECTOR 0x19, ZERO
VECTOR 0x1a, ZERO
VECTOR 0x1b, ZERO
VECTOR 0x1c, ZERO
VECTOR 0x1d, ZERO
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO
VECTOR 0x20, ZERO
