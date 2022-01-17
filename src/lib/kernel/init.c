#include "../../include/kernel/init.h"
#include "../../include/kernel/print.h"
#include "../../include/kernel/interrupt.h"


void init_all() {
    put_str("init_all\n");
    idt_init();
}
