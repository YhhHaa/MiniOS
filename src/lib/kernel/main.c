#include "../../include/kernel/print.h"
#include "../../include/kernel/init.h"
#include "../../include/kernel/debug.h"


void main(void) {
    put_str("I am in kernel now!\n");
    init_all();
    ASSERT(1==2);
    while(1);
    return 0;
}
