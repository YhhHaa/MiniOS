#ifndef __USERPROG_SYSCALL_INIT_H
#define __USERPROG_SYSCALL_INIT_H

#include "../kernel/stdint.h"
#include "../thread/thread.h"
#include "../user/syscall.h"

// 定义syscall数组
#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid(void);
void syscall_init(void);

#endif
