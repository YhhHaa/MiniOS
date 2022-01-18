#include "../../include/kernel/interrupt.h"
#include "../../include/kernel/stdint.h"
#include "../../include/kernel/global.h"
#include "../../include/kernel/io.h"


#define PIC_M_CTRL 0x20 // 主片控制端口
#define PIC_M_DATA 0x21 // 主片数据端口
#define PIC_S_CTRL 0xa0 // 从片控制端口
#define PIC_S_DATA 0xa1 // 从片数据端口
#define IDT_DESC_CNT 0x21 // 目前支持的中断数
#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0": "=g"(EFLAG_VAR))


// 中断门描述符结构体
struct gate_desc {
    uint16_t func_offset_low_word; // 低16位偏移量
    uint16_t selector; // 选择子

    uint8_t dcount;  // 门描述符第4字节, 固定值

    uint8_t attribute; // 属性位
    uint16_t func_offset_high_word; // 高16位偏移量
};


// 静态函数申明
char* intr_name[IDT_DESC_CNT]; // 用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT]; // 异常地址映射表
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];


// 创建中断门描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
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


// 通用的中断处理函数, 一般用在异常出现时的处理
static void general_intr_handler(uint8_t vec_nr) {
    // IRQ7和IRQ15产生伪中断, 无需处理
    if(vec_nr == 0x27 || vec_nr == 0x2f) {
        return;
    }
    put_str("int vertor: 0x");
    put_int(vec_nr);
    put_char('\n');
}


// 完成一般的中断处理函数注册及异常名称注册
static void exception_init(void) {
    int i;
    for(i = 0; i < IDT_DESC_CNT; i++) {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}


enum intr_status intr_enable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        return old_status;
    } else {
        old_status = INTR_OFF;
        asm volatile("sti");
        return old_status;
    }
}

enum intr_status intr_disable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        asm volatile("cli": : : "memory");
        return old_status;
    } else {
        old_status = INTR_OFF;
        return old_status;
    }
}

enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable(): intr_disable();
}

enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags)? INTR_ON: INTR_OFF;
}

// 完成有关中断的所有初始化操作
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0": : "m" (idt_operand));
    put_str("idt_init done\n");
}
