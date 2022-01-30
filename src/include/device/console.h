#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H


#include "../kernel/print.h"
#include "../kernel/stdint.h"
#include "../thread/sync.h"
#include "../thread/thread.h"


void console_init();
void console_release();
void console_put_str(char* str);
void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);


#endif
