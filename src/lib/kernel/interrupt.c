#include "../../include/kernel/interrupt.h"
#include "../../include/kernel/stdint.h"
#include "../../include/kernel/global.h"
#include "../../include/kernel/io.h"


#define PIC_M_CTRL 0x20 // 主片控制端口
#define PIC_M_DATA 0x21 // 主片数据端口
#define PIC_S_CTRL 0xa0 // 从片控制端口
#define PIC_S_DATA 0xa1 // 从片数据端口
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


// 初始化可编程中断控制器8295A
static void pic_init(void) {
    // 初始化主片
    outb(PIC_M_CTRL, 0x11); // ICW1: 边沿触发, 级联模式, 需要ICW4
    outb(PIC_M_DATA, 0x20); // ICW2: 起始中断向量号为0x20
    outb(PIC_M_DATA, 0x04); // ICW3: IR2接从片
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    // 初始化从片
    outb(PIC_S_CTRL, 0x11); // ICW1: 边沿触发, 级联模式, 需要ICW4
    outb(PIC_S_DATA, 0x28); // ICW2: 起始中断向量号为0x28
    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    // 打开主片上IRO, 只接受时钟产生的中断
    outb(PIC_M_DATA, 0xfe); // 主片OCW1: 不屏蔽时钟中断, 屏蔽其他中断
    outb(PIC_S_DATA, 0xff); // 从片OCW1: 全部屏蔽中断

    put_str("pic_init done\n");
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
