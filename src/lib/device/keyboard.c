#include "../../include/device/keyboard.h"

// 定义以下变量记录相应键是否按下的状态, ext_scancode用于记录makecode是否以0xe0开头
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

/* 以通码make_code为索引的二维数组 */
static char keymap[][2] = {
/* 扫描码   未与shift组合的ASCII  与shift组合的ASCII*/
/* ---------------------------------- */
/* 0x00 */	{0,	0},		
/* 0x01 */	{esc,	esc},		
/* 0x02 */	{'1',	'!'},		
/* 0x03 */	{'2',	'@'},		
/* 0x04 */	{'3',	'#'},		
/* 0x05 */	{'4',	'$'},		
/* 0x06 */	{'5',	'%'},		
/* 0x07 */	{'6',	'^'},		
/* 0x08 */	{'7',	'&'},		
/* 0x09 */	{'8',	'*'},		
/* 0x0A */	{'9',	'('},		
/* 0x0B */	{'0',	')'},		
/* 0x0C */	{'-',	'_'},		
/* 0x0D */	{'=',	'+'},		
/* 0x0E */	{backspace, backspace},	
/* 0x0F */	{tab,	tab},		
/* 0x10 */	{'q',	'Q'},		
/* 0x11 */	{'w',	'W'},		
/* 0x12 */	{'e',	'E'},		
/* 0x13 */	{'r',	'R'},		
/* 0x14 */	{'t',	'T'},		
/* 0x15 */	{'y',	'Y'},		
/* 0x16 */	{'u',	'U'},		
/* 0x17 */	{'i',	'I'},		
/* 0x18 */	{'o',	'O'},		
/* 0x19 */	{'p',	'P'},		
/* 0x1A */	{'[',	'{'},		
/* 0x1B */	{']',	'}'},		
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},		
/* 0x1F */	{'s',	'S'},		
/* 0x20 */	{'d',	'D'},		
/* 0x21 */	{'f',	'F'},		
/* 0x22 */	{'g',	'G'},		
/* 0x23 */	{'h',	'H'},		
/* 0x24 */	{'j',	'J'},		
/* 0x25 */	{'k',	'K'},		
/* 0x26 */	{'l',	'L'},		
/* 0x27 */	{';',	':'},		
/* 0x28 */	{'\'',	'"'},		
/* 0x29 */	{'`',	'~'},		
/* 0x2A */	{shift_l_char, shift_l_char},	
/* 0x2B */	{'\\',	'|'},		
/* 0x2C */	{'z',	'Z'},		
/* 0x2D */	{'x',	'X'},		
/* 0x2E */	{'c',	'C'},		
/* 0x2F */	{'v',	'V'},		
/* 0x30 */	{'b',	'B'},		
/* 0x31 */	{'n',	'N'},		
/* 0x32 */	{'m',	'M'},		
/* 0x33 */	{',',	'<'},		
/* 0x34 */	{'.',	'>'},		
/* 0x35 */	{'/',	'?'},
/* 0x36	*/	{shift_r_char, shift_r_char},	
/* 0x37 */	{'*',	'*'},    	
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},		
/* 0x3A */	{caps_lock_char, caps_lock_char}
/*其它按键暂不处理*/
};


// 键盘中断处理程序
static void intr_keyboard_handler(void) {
	// 这次中断发生前的上一次中断, 以下三个键是否按下
	bool ctrl_down_last = ctrl_status;
	bool shift_down_last = shift_status;
	bool caps_lock_last = caps_lock_status;

	bool break_code;
	uint16_t scancode = inb(KBD_BUF_PORT); // 获取扫描码

	/* 若扫描码以e0开头, 表示此键的按下将产生多个扫描码,
	则结束此次中断处理函数, 等待下一个扫描码进来 */
	if(scancode == 0xe0) {
		ext_scancode = true; // 打开e0标记
		return;
	}

	/* 如果上次以0xe0开头, 则将扫描码合并 */
	if(ext_scancode) {
		scancode = ((0xe000) | scancode);
		ext_scancode = false; // 关闭e0标记
	}

	break_code = ((scancode & 0x0080) != 0); // 断码的第8位为1
	if(break_code) { // 若断码break_code
		/* 由于ctrl_r和alt_r的make_code和break_code都是两字节,
		所以可用下面的方法取make_code, 多字节的扫描码暂不处理
		断码第8位为1, 通码第8位为0 */
		uint16_t make_code = (scancode &= 0xff7f);

		/* 若是任意以下三个键弹起了, 将状态置为false */
		if(make_code == ctrl_l_make || make_code == ctrl_r_make) {
			ctrl_status = false;
		} else if(make_code == shift_l_make || make_code == shift_r_make) {
			shift_status = false;
		} else if(make_code == alt_l_make || make_code == alt_r_make) {
			alt_status = false;
		}
		return; // 结束此次中断处理程序
	}/* 若为通码, 只处理数组中定义的键以及alt_right和ctrl键, 全是make_code */
	else if((scancode > 0x00 && scancode < 0x3b) || (scancode == alt_r_make) || (scancode == ctrl_r_make)) {
		bool shift = false; // 判断是否与shift组合, 用来在一维数组中索引对应的字符
		if((scancode < 0x0e) || (scancode == 0x29) || /* 数字0-9 字符- = < 0x0e 0x29 ` */ \
			(scancode == 0x1a) || (scancode == 0x1b) || /* [] */\
			(scancode == 0x2b) || (scancode == 0x27) || /* \\ ; */\
			(scancode == 0x28) || (scancode == 0x33) || /* \ , */\
			(scancode == 0x34) || (scancode == 0x35)) /* . / */{
				if(shift_down_last) {
					shift = true;
				}
			} else { // 默认为字母键
				if(shift_down_last && caps_lock_last) { // 如果shift和capslock同时按下
					shift = false;
				} else if(shift_down_last || caps_lock_last) { // 如果shift和capslock任意按下
					shift = true;
				} else {
					shift = false;
				}
			}

		uint8_t index = (scancode &= 0x00ff); // 将扫描码高字节置0, 主要是针对高字节的是e0的扫描码
		char cur_char = keymap[index][shift]; // 在数组中找到对应的字符

		if(cur_char) { // 只处理ASCII不为0的键
			put_char(cur_char);
			return;
		}

		// 记录本次是否按下了下面几类控制键之一, 供下次键入时判断组合键
		if(scancode == ctrl_l_make || scancode == ctrl_r_make) {
			ctrl_status = true;
		} else if(scancode == shift_l_make || scancode == shift_r_make) {
			shift_status = true;
		} else if(scancode == alt_l_make || scancode == alt_r_make) {
			alt_status = true;
		} else if(scancode == caps_lock_make) {
			caps_lock_status = !caps_lock_status;
		}
	} else {
		put_str("unknow key\n");
	}
}

// 注册键盘中断处理程序
void keyboard_init() {
	put_str("keyboard init start\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done\n");
}
