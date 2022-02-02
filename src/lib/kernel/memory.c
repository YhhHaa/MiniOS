#include "../../include/kernel/memory.h"


// 物理内存池结构体
struct pool {
	struct bitmap pool_bitmap; //本内存池用到的位图结构, 用于管理物理内存
	uint32_t phy_addr_start; // 本内存池所管理物理内存的起始地址
	uint32_t pool_size; // 本内存池字节容量
	struct lock lock; // 申请内存时互斥
};
struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr; // 此结构用来给内核分配虚拟地址


/* 在pf表示的虚拟内存池中申请pg_cnt个虚拟页, 
成功返回虚拟页的起始地址, 失败则返回NULL */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if(pf == PF_KERNEL) { // 内核内存池
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1) {
			return NULL;
		}
		while(cnt < pg_cnt) {
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	} else { // 用户内存池
		struct task_struct* cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr->vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1) {
			return NULL;
		}
		while(cnt < pg_cnt) {
			bitmap_set(&cur->userprog_vaddr->vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = cur->userprog_vaddr->vaddr_start + bit_idx_start * PG_SIZE;

		// (0xc0000000 - PG_SIZE)作为用户3级栈已经在start_process被分配
		ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
	}
	return (void*)vaddr_start;
}

/* 得到虚拟地址对应的pte指针, 注意loader.S中在最后一个页目录项中定义的是页目录表自己的物理地址
因此, 高10位索引页目录项pde到最后一项, 1023项即0x3ff, 移动到高10位变为0xffc00000, 取得的值为boot.inc中定义的0x1000000
中间10位是页表项的索引, 此时已经访问到了页目录表当前页表, 需要虚拟地址的高10位找到原来的页目录项, 因此(vaddr & 0xffc00000) >> 10作为新地址的中间10位
最后使用PTE(vaddr)获得pte索引再*4模拟虚拟地址低12位*/
uint32_t* pte_ptr(uint32_t vaddr) {
	uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}

/* 得到虚拟地址对应的pde指针, 注意页表映射中高20位0xfffff表示访问到最后一个页目录项, 高10位0x3ff, 中10位0x3ff
随后只需要得到原来的pde索引即可, 位于原地址的高10位 */
uint32_t* pde_ptr(uint32_t vaddr) {
	uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
	return pde;
}

/* 在m_pool指向的物理内存池中分配一个物理页,
成功则返回页框的物理地址, 失败则返回NULL*/
static void* palloc(struct pool* m_pool) {
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(bit_idx == -1) {
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
	return (void*)page_phyaddr;
}

// 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);

	// 判断页目录项P位存在
	if(*pde & 0x00000001) {
		// 判断页表项P位是否存在, 如果存在则引发panic.
		ASSERT(!(*pte & 0x00000001));

		// 如果页表不存在
		if(!(*pte & 0x00000001)) {
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		} else { // 目前无法执行到此处, 因为ASSERT断言判断
			PANIC("pte repeat");
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}
	} else { // 如果页目录项不存在
		// 先创建页目录项内容, 申请页表, 再创建页表项
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

		/* 将分配到的物理页地址对应物理内存清0，防止陈旧数据变成页表项干扰页表
		访问pde对应的物理地址用pte取高20位即可, 因为pte基于该pde对应的物理地址再寻址,
		把低12位置0便是该pde对应的物理页的起始 */
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);

		// 如果pte内的物理页存在, 则引发断言
		ASSERT(!(*pte & 0x00000001));

		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}

// 分配pg_cnt个页空间, 成功则返回起始虚拟地址, 失败时返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);
	/* 通过vaddr_get在虚拟内存池中申请虚拟地址
	通过palloc在物理内存池中申请物理页
	通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
	 */
	void* vaddr_start = vaddr_get(pf, pg_cnt);
	if(vaddr_start == NULL) {
		return NULL;
	}

	uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

	// 因为虚拟地址是连续的, 但是物理地址可以不连续, 因此逐个映射
	while(cnt-- > 0) {
		void* page_phyaddr = palloc(mem_pool);
		if(page_phyaddr == NULL) { // 失败时要将曾经申请的虚拟地址和物理页全部回滚, 后续完成内存回收再处理
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr); // 在页表中映射
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

// 从内核物理池申请pg_cnt页内存, 成功返回其虚拟地址, 失败返回NULL
void* get_kernel_pages(uint32_t pg_cnt) {
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL) { // 若分配地址不为空, 将页框清0后返回
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	return vaddr;
}

/* 在用户空间中申请4k内存, 并返回其虚拟地址 */
void* get_user_pages(uint32_t pg_cnt) {
	lock_acquire(&user_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	memset(vaddr, 0, pg_cnt * PG_SIZE);
	lock_release(&user_pool.lock);
	return vaddr;
}

/* 将地址vaddr与pf池中的物理地址关联, 仅支持一页空间分配 */
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool: &user_pool;

	lock_acquire(&mem_pool->lock);

	struct task_struct* cur = running_thread();
	int32_t bit_idx = -1;

	// 申请虚拟地址
	/* 若当前是用户进程申请用户内存, 就修改用户进程自己的虚拟地址位图 */
	if(cur->pgdir != NULL && pf == PF_USER) {
		bit_idx = (vaddr - cur->userprog_vaddr->vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&cur->userprog_vaddr->vaddr_bitmap, bit_idx, 1);
	} else if(cur->pgdir == NULL && pf == PF_KERNEL){
	/* 如果是内核线程申请内核内存, 就修改kernel_vaddr */
		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
	} else {
		PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
	}

	// 申请物理地址
	void* page_phyaddr = palloc(mem_pool);
	if(page_phyaddr == NULL) {
		return NULL;
	}

	// 完成映射
	page_table_add((void*)vaddr, page_phyaddr);
	
	lock_release(&mem_pool->lock);
	return (void*)vaddr;
}

/* 得到虚拟地址映射到物理地址 */
uint32_t addr_v2p(uint32_t vaddr) {
	uint32_t* pte = pte_ptr(vaddr);
	/* (*pte)的值是页表所在的物理页框地址,
	去掉其低12位的页表项属性+虚拟地址vaddr的低12位偏移量 */
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

// 初始化内存池, 参数为loader加载器0xb00处的物理内存大小
static void mem_pool_init(uint32_t all_mem) {
	put_str("mem_pool_init start\n");

	/* 
	记录页目录项和页表占用的字节大小:
		页目录大小为4KB
		页目录第0和768个页目录项指向同一个页表
		第769-1022共指向254个页表
		相加则一共占用256*PG_SIZE=2MB
	 */
	uint32_t page_table_size = PG_SIZE * 256;

	/* 
	记录当前已经使用的内存字节数:
		页表大小page_table_size
		低段1MB内存
	 */
	uint32_t used_mem = page_table_size + 0x00100000;

	// 记录剩余的总物理内存
	uint32_t free_mem = all_mem - used_mem;

	// 记录所有剩余的物理页数
	uint16_t all_free_pages = free_mem / PG_SIZE;

	// 可用物理页的数量
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;

	// bitmap的字节长度
	uint32_t kbm_length = kernel_free_pages / 8;
	uint32_t ubm_length = user_free_pages / 8;

	// 内存池的起始地址
	uint32_t kp_start = used_mem;
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

	// 内存储初始化
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

	// 打印内存池信息
	put_str("kernel_pool_bitmap_start:");
	put_int((int)kernel_pool.pool_bitmap.bits);
	put_str("kernel_pool_phy_addr_start:");
	put_int((int)kernel_pool.phy_addr_start);
	put_str("\n");
	put_str("user_pool_bitmap_start:");
	put_int((int)user_pool.pool_bitmap.bits);
	put_str("user_pool_phy_addr_start:");
	put_int((int)user_pool.phy_addr_start);
	put_str("\n");

	// 初始化锁
	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);

	// 初始化位图将位图置0
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	// 初始化内核虚拟地址位图, 维护内核堆的虚拟地址, 与内核内存池大小一致, 起始地址紧跟在物理内存池位图之后
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("mem_pool_init done\n");
}

void mem_init(void) {
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
