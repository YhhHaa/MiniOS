#ifndef __KERNEL_INIT_H
#define __KERNEL_INIT_H


#include "../kernel/print.h"
#include "../kernel/interrupt.h"
#include "../device/timer.h"
#include "../kernel/memory.h"
#include "../thread/thread.h"

void init_all(void);


#endif