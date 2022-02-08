#ifndef __KERNEL_INIT_H
#define __KERNEL_INIT_H


#include "../kernel/print.h"
#include "../kernel/interrupt.h"
#include "../device/timer.h"
#include "../kernel/memory.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/tss.h"
#include "../userprog/syscall-init.h"

void init_all(void);


#endif