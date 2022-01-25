#include "../../include/device/timer.h"


static void frequency_set(uint8_t counter_port, \
                          uint8_t counter_no,
                          uint8_t rwl,
                          uint8_t counter_mode,
                          uint16_t counter_value) {
    // 写入控制字寄存器内容
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    // 将控制字寄存器内容写入8253计数器0x40端口的计数器0
    outb(counter_port, (uint8_t)counter_value);
    outb(counter_port, (uint8_t)counter_value >> 8);
}


uint32_t ticks; // 自内核开启中断以来总共的嘀嗒数

// 时钟的中断处理函数
static void intr_timer_handler(void) {
	struct task_struct* cur_thread = running_thread();
	ASSERT(cur_thread->stack_magic == 0x19870916);

	cur_thread->elapsed_ticks++; // 记录此线程占用cpu时间
	ticks++; // 记录总嘀嗒数

	if(cur_thread->ticks == 0) { // 若进程时间片用完, 调度新的进程上cpu
		schedule();
	} else { // 将当前进程时间片-1
		cur_thread->ticks--;
	}
}

void timer_init() {
    put_str("timer_init start\n");
    frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
    put_str("timer_init done\n");
}