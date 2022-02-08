#include "../../include/thread/thread.h"


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

// 分配pid
static pid_t allocate_pid(void) {
	static pid_t next_pid = 0;
	lock_acquire(&pid_lock);
	next_pid++;
	lock_release(&pid_lock);
	return next_pid;
}


// 初始化线程基本信息PCB
void init_thread(struct task_struct* pthread, char* name, int prio) {
	memset(pthread, 0, sizeof(*pthread)); // 清空pcb内容

	pthread->pid = allocate_pid(); // 分配pid
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

	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag)); // 确保之前不在队列中
	list_append(&thread_ready_list, &thread->general_tag); // 加入就绪队列
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

// 读写就绪队列, 实现任务调度
void schedule() {
	ASSERT(intr_get_status() == INTR_OFF);

	struct task_struct* cur = running_thread();

	// 将当前运行的线程换下
	if(cur->status == TASK_RUNNING) {
		// 如果是cpu时间片到了, 将其加入就绪队列尾
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	} else {
		// 若是需要发生某事件后才能上cpu运行, 则不需要将其加入队列中, 因为当前线程不在就绪队列中
	}

	// 将新线程换上
	ASSERT(!list_empty(&thread_ready_list));
	thread_tag = NULL; // thread_tag清空
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
	next->status = TASK_RUNNING;

	// 激活任务页表
	process_activate(next);

	switch_to(cur, next);
}

// 初始化线程环境
void thread_init(void) {
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	lock_init(&pid_lock);
	make_main_thread();
	put_str("thread_init done\n");
}

// 当前线程将自己阻塞, 标志其状态为stat
void thread_block(enum task_status stat) {
	// stat取值为TASK_RUNNING, TASK_WAITING, TASK_HANGING不被调度
	ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WATTING) || (stat == TASK_HANGING));

	// 关闭中断
	enum intr_status old_status = intr_disable();

	// 切换当前线程到其他队列状态
	struct task_struct* cur_thread = running_thread();
	cur_thread->status = stat;
	schedule(); // 将当前线程换下处理器

	// 恢复中断
	intr_set_status(old_status);
}

// 将线程pthread解除阻塞, 放到就绪队列中
void thread_unblock(struct task_struct* pthread) {
	// 关闭中断
	enum intr_status old_status = intr_disable();

	// stat取值为TASK_RUNNING, TASK_WAITING, TASK_HANGING不被调度
	ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WATTING) || (pthread->status == TASK_HANGING));

	if(pthread->status != TASK_READY) {
		ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
		if(elem_find(&thread_ready_list, &pthread->general_tag)) {
			PANIC("thread_unblock: blocked thread in ready_list\n");
		}
		list_push(&thread_ready_list, &pthread->general_tag);
		pthread->status = TASK_READY;
	}

	// 恢复中断
	intr_set_status(old_status);
}
