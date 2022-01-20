#include "../../include/kernel/memory.h"
#include "../../include/kernel/stdint.h"
#include "../../include/kernel/print.h"
#include "../../include/kernel/bitmap.h"


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


// 物理内存池结构体
struct pool {
	struct bitmap pool_bitmap; //本内存池用到的位图结构, 用于管理物理内存
	uint32_t phy_addr_start; // 本内存池所管理物理内存的起始地址
	uint32_t pool_size; // 本内存池字节容量
};
struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr; // 此结构用来给内核分配虚拟地址

// 初始化内存池, 参数为loader加载器0xb00处的物理内存大小
static void mem_pool_init(uint32_t all_mem) {
	put_str("mem_pool_init start\n");

	/* 
	记录页目录项和页表占用的字节大小:
		页目录大小为4KB
		页目录第0和768个页目录项指向同一个页表
		第769-1022共指向254和页表
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

void mem_init() {
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
