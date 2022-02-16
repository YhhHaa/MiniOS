#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"
#include "memory.h"
#include "fs.h"
#include "string.h"
#include "fs.h"
#include "dir.h"
#include "shell.h"

void init(void);
extern void cls_screen(void);
extern struct ide_channel channels[2]; // 有两个ide通道

int main(void) {
	put_str("I am kernel\n");
	init_all();
	/* ******* 写入应用程序 *********** */
	uint32_t file_size = 6172;
	uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
	struct disk* sda = &channels[0].devices[0];
	void* prog_buf = sys_malloc(file_size);
	ide_read(sda, 300, prog_buf, sec_cnt);
	int32_t fd = sys_open("/prog_no_arg", O_CREAT | O_RDWR);
	if (fd != -1) {
		if (sys_write(fd, prog_buf, file_size) == -1) {
			printk("file write error\n");
			while (1);
		}
	}
	/* ******* 写入应用程序 *********** */

	cls_screen();
	console_put_str("[yhhhaa@localhost /]$ ");
	intr_enable(); // 打开中断
	
	while(1);
	return 0;
}

/* init 进程 */
void init(void) {
	uint32_t ret_pid = fork();
	if (ret_pid) {
		while(1);
	} else {
		my_shell();
	}
	while(1);
}