#include "../../include/device/keyboard.h"


// 键盘中断处理程序
static void intr_keyboard_handler(void) {
	uint8_t scancode = inb(KBD_BUF_PORT); // 必须读取输出缓冲区寄存器, 否则8042不再响应中断
	put_int(scancode);
	return;
}

// 注册键盘中断处理程序
void keyboard_init() {
	put_str("keyboard init start\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done\n");
}
