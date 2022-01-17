#include "../../include/kernel/print.h"
#include "../../include/kernel/init.h"


void main(void) {
    put_str("I am in kernel now!\n");
    init_all();
    asm volatile("sti");
    while(1);
}
