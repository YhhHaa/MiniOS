#include "../../include/kernel/init.h"


void init_all() {
    put_str("init_all\n");
    idt_init(); // 初始化中断
    timer_init(); // 初始化PIT
	mem_init(); // 初始化内存池
	thread_init(); // 初始化主线程
	console_init(); // 初始化控制台
	keyboard_init(); // 初始化键盘中断
}
