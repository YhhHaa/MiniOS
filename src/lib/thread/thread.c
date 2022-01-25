#include "../../include/thread/thread.h"
#include "../../include/string/string.h"
#include "../../include/kernel/global.h"
#include "../../include/kernel/memory.h"
#include "../../include/kernel/interrupt.h"
#include "../../include/kernel/debug.h"


struct task_struct* main_thread; // 主线程PCB
struct list thread_ready_list; // 就绪队列
struct list thread_all_list; // 所有任务队列
static struct list_elem* thread_tag; // 用于保存队列中的线程结点

extern void switch_to(struct task_struct* cur, struct task_struct* next);


// 获取当前进程PCB指针
struct task_struct* running_thread() {
	uint32_t esp;
	asm("mov %%esp, %0": "=g" (esp));
	// 取esp使用的PCB0级栈的高20位, 即PCB起始地址
	return (struct task_struct*)(esp & 0xfffff000);
}

// 由kernel_thread去执行function(func_arg)
static void kernel_thread(thread_func* function, void* func_arg) {
	intr_enable(); // 执行前开中断, 避免后面的时钟中断被屏蔽, 无法调度其他线程
	function(func_arg);
}

/* 初始化线程栈thread_stack
将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
	// 先预留中断使用栈的空间
	pthread->self_kstack -= sizeof(struct intr_stack);

	// 再预留处线程栈空间
	pthread->self_kstack -= sizeof(struct thread_stack);

	// 初始化线程栈
	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;
	kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程基本信息PCB
void init_thread(struct task_struct* pthread, char* name, int prio) {
	memset(pthread, 0, sizeof(*pthread)); // 清空pcb内容
	strcpy(pthread->name, name);

	if(pthread == main_thread) {
		pthread->status = TASK_RUNNING;
	} else {
		pthread->status = TASK_READY;
	}

	pthread->priority = prio;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL; // 线程没有自己的地址空间
	pthread->stack_magic = 0x19870916; // 自定义魔数, 用于防止栈溢出到PCB
}

// 创建一优先级为prio的线程, 线程名为name, 执行函数是function(func_arg)
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
	// 申请PCB结构体的内存: 虚拟地址申请以及物理地址申请以及映射
	struct task_struct* thread = get_kernel_pages(1);

	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);

	ASSERT(!elem_find(&thread_ready_list, &thread->genral_tag)); // 确保之前不在队列中
	list_append(&thread_ready_list, &thread->genral_tag); // 加入就绪队列
	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag); // 加入全部线程队列

	return thread;
}

static void make_main_thread(void) {
	/* 主main线程早已运行, 其pcb为loader.S进入内核mov esp, 0xc009f000预留了PCB
	PCB地址位0xc009e000 */
	main_thread = running_thread();
	init_thread(main_thread, "main", 31);

	ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag);
}
