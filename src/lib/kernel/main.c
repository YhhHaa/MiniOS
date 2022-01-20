#include "../../include/kernel/print.h"
#include "../../include/kernel/init.h"
#include "../../include/kernel/debug.h"


void main(void) {
    put_str("I am in kernel now!\n");
    init_all();
    while(1);
    return 0;
}
