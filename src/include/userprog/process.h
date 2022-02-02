#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H

#include "../thread/thread.h"
#include "../device/console.h"
#include "../kernel/debug.h"

/* 效仿c程序内存布局, 最高为栈空间, 接着为堆空间
命令行参数压入栈顶,
由于用户空间最高地址0xc0000000, 且内存管理模块返回的时内存空间的下边界 */
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
/* 用户进程虚拟地址的起始地址,
该值是linux用户程序入口地址 */
#define USER_VADDR_START 0x8048000
#define default_prio 100

void start_process(void* filename_);
void page_dir_activate(struct task_struct* p_thread);
void process_activate(struct task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);
void process_execute(void* filename, char* name);
void extern intr_exit(void);

#endif