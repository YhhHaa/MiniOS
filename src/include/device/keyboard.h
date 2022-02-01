#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

#include "../kernel/print.h"
#include "../kernel/interrupt.h"
#include "../kernel/io.h"
#include "../kernel/global.h"

#define KBD_BUF_PORT 0x60 // 键盘buffer寄存器端口号为0x60

// 转义字符定义部分控制字符
#define esc '\033'
#define backspace '\b'
#define tab '\t'
#define enter '\r'
#define delete '\177'

// 控制不可见字符一律定义为不可见
#define char_invisible 0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible

// 定义控制字符的通码和断码
#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_bread 0xe09d
#define caps_lock_make 0x3a


static void intr_keyboard_handler(void);
void keyboard_init();

#endif