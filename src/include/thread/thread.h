#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H


#include "../kernel/stdint.h"

#define PG_SIZE 4096

// 自定义通用函数类型, 在很多线程函数中作为形参类型
typedef void thread_func(void*);

// 进场或线程的状态
enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WATTING,
	TASK_HANGING,
	TASK_DIED
};

// 中断栈
struct intr_stack {
	uint32_t vec_no; // kernel.S宏VECTOR中push %1压入的中断号
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy; // popad不会弹出esp, 因此压入自定义的esp
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	// 以下是cpu从低特权级到高特权级时压入
	uint32_t err_code;
	void (*eip) (void);
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss;
};

// 线程栈, 仅用于在switch_to切换线程时保存线程环境
struct thread_stack {
	// 应用程序二进制接口, 设定编译方面的约定, 这5个寄存器在被调用函数结束后需要和主调函数保持一致
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;

	// 线程第一次执行时, eip指向待调用的函数kernel_thread, 其他时候指向switch_to的返回地址
	void (*eip) (thread_func* func, void* func_arg);

	// 以下仅供第一次被调度上cpu时执行
	void (*unused_retaddr); // 参数unused_ret只为占位置充数为返回地址
	thread_func* function; // 由kernel_thread所调用的函数名
	void* func_arg; // 由kernel_thread所调用的函数所需的参数
};

// 进程或线程的pcb, 程序控制块
struct task_struct {
	uint32_t* self_kstack; // 各内核线程都用自己的内核栈, 自身PCB所在页的顶端
	enum task_status status; // 线程状态
	uint8_t priority; // 线程优先级
	char name[16]; // 任务名
	uint32_t stack_magic; // 栈的边界标记, 用于检测栈的溢出
};


// 函数申明
static void kernel_thread(thread_func* function, void* func_arg);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);

#endif
