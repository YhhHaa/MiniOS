#include "../../include/userprog/process.h"


/* 初始化进程PCB的中断栈并跳转至中断结束函数 */
void start_process(void* filename_) {
	void* function = filename_;

	// 初始化进程PCB的中断栈
	struct task_struct* cur = running_thread();
	cur->self_kstack += sizeof(struct thread_stack); // 得到中断栈栈顶地址
	struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack; // 设置为中断栈
	proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
	proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
	proc_stack->gs = 0; // 用户态无需操作显存, 初始化为0
	proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
	proc_stack->eip = function; // 待执行的用户程序地址
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE); // 分配栈
	proc_stack->ss = SELECTOR_U_DATA;

	// 加载中断栈并跳转至中断结束
	asm volatile ("movl %0, %%esp; jmp intr_exit;" : : "g" (proc_stack) : "memory");
}

/* cr3中激活用户态进程或内核态线程的页目录表 */
void page_dir_activate(struct task_struct* p_thread) {
	/* 执行此函数时, 当前任务可能是线程,
	之所以对线程也要重新安装页表, 原因时上一次被调度的可能是进程,
	如果不恢复页表的话, 线程就会使用进程的页表了 */

	/* 若为内核线程, 需要重新填充页表为0x100000, 虚拟地址为0xc00100000, 是页目录表地址
	默认为内核的页目录物理地址, 也就是内核线程所用的页目录表,
	若为用户态进程, 需要获得页目录表的物理地址并装载 */
	uint32_t pagedir_phy_addr = 0x100000;
	if(p_thread->pgdir != NULL) { // 用户态进程有自己的页目录表
		pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir); // 获得页目录表物理地址
	}
	asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory"); // 页目录表物理地址装载至cr3寄存器
}

/* 调用page_dir_activate激活页目录表,
如果是用户级进程, 还需要更新tss中的esp0, 保证处理器进入中断时能拿到正确的特权级栈 */
void process_activate(struct task_struct* p_thread) {
	ASSERT(p_thread != NULL);

	page_dir_activate(p_thread); // 激活进程或线程的页目录表

	/* 内核线程特权级本身就是0, 处理器进入中断不会从tss中获取0特权级栈地址.
	而用户级进程特权级为3, 进入中断处理器会从tss中获取3特权级栈地址 */
	if(p_thread->pgdir) {
		update_tss_esp(p_thread); // 更新该进程的esp0, 用于此进程被中断时保留上下文
	}
}

/* 用户进程创建页目录表 */
uint32_t* create_page_dir(void) {
	// 用户进程的页表不能让用户直接访问到, 所以使用内核物理池分配其虚拟地址空间
	uint32_t* page_dir_vaddr = get_kernel_pages(1);

	// 物理内存分配失败
	if(page_dir_vaddr == NULL) {
		console_put_str("create_page_dir: get_kernel_page failed!");
		return NULL;
	}

	/* 1. 先复制页表
	page_dir_vaddr + 0x300 * 4是内核页目录表的第768项, 此处为进程页目录表的目标地址
	将内核768~1023复制到每个进程的页目录表中, 这样就可以共享内核的数据
	创建进程在内核中完成, 0xfffff000通过虚拟地址访问内核页目录表的第0个页目录项
	1024个字节/4 = 256个页目录项 */
	memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);

	/* 2. 更新页目录表最后一项为用户进程自己的页目录表物理地址 */
	uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr); // 得到页目录表的物理地址
	page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1; // 修改1023项为自身

	return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
	user_prog->userprog_vaddr->vaddr_start = USER_VADDR_START; // 定义虚拟内存的起始地址
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE); // 进位思想向上取整得到记录位图需要的内存页框数
	user_prog->userprog_vaddr->vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt); // 为位图分配内存
	user_prog->userprog_vaddr->vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8; // 位图的长度
	bitmap_init(&user_prog->userprog_vaddr->vaddr_bitmap); // 位图初始化所有位置0
}

void process_execute(void* filename, char* name) {
	struct task_struct* thread = get_kernel_pages(1); // 申请进程PCB虚拟地址
	init_thread(thread, name, default_prio); // 初始化进程PCB
	create_user_vaddr_bitmap(thread); // 创建用户进程虚拟地址位图
	thread_create(thread, start_process, filename); // 初始化进程栈
	thread->pgdir = create_page_dir(); // 新建进程页目录表

	// 装载进程
	enum intr_status old_status = intr_disable();

	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);

	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);

	intr_set_status(old_status);
}
