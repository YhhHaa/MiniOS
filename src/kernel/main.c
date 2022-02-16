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

int main(void) {
   put_str("I am kernel\n");
   init_all();
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