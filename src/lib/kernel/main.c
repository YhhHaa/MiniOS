#include "../../include/kernel/print.h"


void main(void) {
    put_str("I am in kernel now!\n");
    put_int(0);
    put_char('\n');
    put_int(100);
    while(1);
}
