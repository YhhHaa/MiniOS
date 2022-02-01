#ifndef __KERNEL_MAIN_H
#define __KERNEL_MAIN_H

#include "../kernel/print.h"
#include "../kernel/init.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/ioqueue.h"
#include "../device/keyboard.h"

void k_thread_a(void*);
void k_thread_b(void*);

#endif