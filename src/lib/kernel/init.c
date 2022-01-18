#include "../../include/kernel/init.h"
#include "../../include/kernel/print.h"
#include "../../include/kernel/interrupt.h"
#include "../../include/device/timer.h"


void init_all() {
    put_str("init_all\n");
    idt_init(); // 初始化中断
    timer_init(); // 初始化PIT
}
