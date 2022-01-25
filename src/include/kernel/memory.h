#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H


#include "../kernel/stdint.h"
#include "../kernel/print.h"
#include "../kernel/bitmap.h"
#include "../kernel/debug.h"
#include "../string/string.h"
#include "../kernel/global.h"


// 区分在哪个内存池分配内存
enum pool_flags {
	PF_KERNEL = 1, // 内核内存池
	PF_USER = 2, // 用户内存池
};

#define PG_P_1 1 // 页表项或页目录项存在属性位
#define PG_P_0 0
#define PG_RW_R 0 // 读/执行
#define PG_RW_W 2 // 读/写/执行
#define PG_US_S 0 // 系统级
#define PG_US_U 4 // 用户级

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22) // 在页目录表中定位pde
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12) // 在页表中定位pte


// 页尺寸4KB
#define PG_SIZE 4096
/* 
因为0xc009f000是内核主线程栈顶, 0xc009e000是内核主线程pcb, 因为一个pcb要占满1kb
一个页框4KB大小的位图可以表示128MB的内存, 预计使用4页位图, 位图位置安排在地址0xc009e000 - 4kb = 0xc009a000
这样本系统最大支持4个页框的位图, 即512MB
 */
#define MEM_BITMAP_BASE 0xc009a000
/* 
0xc0000000是内核从虚拟地址3G起, 通过页表已经将0xc0000000-0xc00fffff映射到了0x00000000-0x000fffff
低段1MB内存.0x0010_0000是指跨过低段1MB内存, 使虚拟地址在逻辑上连续
注意页目录表0x100000~0x101fff, 堆内存的映射需要绕过页目录表
 */
#define K_HEAP_START 0xc0100000


// 虚拟地址池, 用于虚拟地址管理
struct virtual_addr {
	struct bitmap vaddr_bitmap; // 虚拟地址用到的位图结构
	uint32_t vaddr_start; // 虚拟地址起始地址
};

extern struct pool kernel_pool, user_pool;
extern struct virtual_addr kernel_vaddr;
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
static void* palloc(struct pool* m_pool);
static void page_table_add(void* _vaddr, void* _page_phyaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
static void mem_pool_init(uint32_t all_mem);
void mem_init(void);

#endif