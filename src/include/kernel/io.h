/*   机器模式
    b -- 输出寄存器QImode名称, 即寄存器中最低8位
    w -- 输出寄存器HImode名称, 即寄存器中两个字节的部分
    HImode: 表示一个两字节的整数
    QImode: 表示一个字节的整数
*/

#ifndef __LIB_IO_H
#define __LIB_IO_H


#include "./stdint.h"


// 向端口port写入一个字节
static inline void outb(uint16_t port, uint8_t data) {
    // %b0表示取1个字节, %w1表示取2个字节
    // Nd表示指定端口0-255且用dx寄存器存储
    asm volatile ("outb %b0 %w1": : "a" (data), "Nd" (port));
}


// 将addr处起始的word_cnt个字写入端口port-存于dx寄存器
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
    // +表示此限制又做输入, 又做输出.
    // outsw将ds:esi处的16位内容写入port端口
    asm volatile ("cld; rep outsw": "+S" (addr), "+c" (word_cnt): "d" (port));
}


// 将从端口port读入的1个字节返回
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %w1, %b0": "=a" (data): "Nd" (port));
    return data;
}


// 将从端口port读入的word_cnt个字写入addr
static inline void insw(uint16_t port, const void* addr, uint32_t word_cnt) {
    asm volatile ("cld; rep insw": "+D" (addr), "+c" (word_cnt): "d" (port) : "memory");
}


#endif
