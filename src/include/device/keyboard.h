#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

#include "../kernel/print.h"
#include "../kernel/interrupt.h"
#include "../kernel/io.h"
#include "../kernel/global.h"

#define KBD_BUF_PORT 0x60 // 键盘buffer寄存器端口号为0x60

static void intr_keyboard_handler(void);
void keyboard_init();

#endif