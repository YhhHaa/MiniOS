#include "../../include/userprog/tss.h"


static struct tss tss;

/* 更新tss中的esp0字段的值为pthread的0级栈 */
void update_tss_esp(struct task_struct* pthread) {
	tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

/* 创建gdt描述符 */
static struct gdt_desc make_gdt_desc(uint32_t* desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high) {
	uint32_t desc_base = (uint32_t)desc_addr;
	struct gdt_desc desc;
	desc.limit_low_word = limit & 0x0000ffff; // 段界限低16位
	desc.base_low_word = desc_base & 0x0000ffff; // 段基址低16位
	desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16); // 段基址中8位
	desc.attr_low_byte = (uint8_t)(attr_low); // 属性8位
	desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high)); // 段界限4位与段属性4位
	desc.base_high_byte = desc_base >> 24; // 段基址高16位
	return desc;
}

/* 在gdt中创建tss并重新加载gdt */
void tss_init() {
	put_str("tss_init start\n");

	// 初始化tss
	uint32_t tss_size = sizeof(tss); // 获得tss结构大小
	memset(&tss, 0, tss_size); // 全部置0
	tss.ss0 = SELECTOR_K_STACK; // 初始化0级栈为段选择子指向GDT第三个段描述符
	tss.io_base = tss_size; // 初始化io位图的偏移地址, 表示tss中没有io位图

	/* gdt段基址为0x900, 把tss放到第4个位置, 也就是0x900 + 20的位置 */
	// 在gdt中添加dpl为0的TSS描述符
	*((struct gdt_desc*)0xc0000920) = make_gdt_desc((uint32_t*)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
	// 在gdt中添加dpl为3的数据段和代码段描述符
	*((struct gdt_desc*)0xc0000928) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
	*((struct gdt_desc*)0xc0000930) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

	/* gdt 16位的limit 32位的段基址
	16位表界限 & 32位表的起始地址 */
	uint64_t gdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16)); // 7个描述符大小

	asm volatile ("lgdt %0": : "m" (gdt_operand));
	asm volatile ("ltr %w0": : "r" (SELECTOR_TSS));
	
	put_str("tss_init and ltr done\n");
}