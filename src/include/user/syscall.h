#ifndef __USER_SYSCALL_H
#define __USER_SYSCALL_H

#include "../kernel/stdint.h"

enum SYSCALL_NR { // 存放系统调用子功能号
	SYS_GETPID
};

uint32_t getpid(void);

#endif
