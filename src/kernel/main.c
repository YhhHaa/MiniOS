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

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
extern void init(void);


int main(void) {
   put_str("I am kernel\n");
   init_all();
   intr_enable(); // 打开中断
 
   while(1);
   return 0;
}

/* init 进程 */
void init(void) {
	uint32_t ret_pid = fork();
	if (ret_pid) {
		printf("i am father, my pid is %d, child pid is %d\n", getpid(), ret_pid);
	} else {
		printf("i am child, my pid is %d, ret pid is %d\n", getpid(), ret_pid);
	}
	while(1);
}