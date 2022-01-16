#include "../../include/kernel/interrupt.h"
#include "../../include/kernel/stdint.h"
#include "../../include/kernel/global.h"


#define IDT_DESC_CNT 0x21 // 目前支持的中断数


// 中断门描述符结构体
struct gate_desc {
    uint16_t func_offset_low_word; // 低16位偏移量
    uint16_t selector; // 选择子

    uint8_t dcount;  // 门描述符第4字节, 固定值

    uint8_t attribute; // 属性位
    uint16_t func_offset_high_word; // 高16位偏移量
};


// 静态函数申明
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];


// 创建中断门描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = (uint32_t)function & 0xffff0000 >> 16;
}

// 初始化中断描述符表
static void idt_desc_init(void) {
    int i;
    for(i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("idt_desc_init done\n");
}

// 完成有关中断的所有初始化操作
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();
    pic_init();

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0": : "m" (idt_operand));
    put_str("idt_init done\n");
}
