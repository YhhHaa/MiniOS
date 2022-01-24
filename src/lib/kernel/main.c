#include "../../include/kernel/print.h"
#include "../../include/kernel/init.h"
#include "../../include/kernel/debug.h"
#include "../../include/kernel/memory.h"
#include "../../include/thread/thread.h"


void k_thread_a(void* arg) {
	char* para = arg;
	while(1)
	put_str(para);
}


void main(void) {
    put_str("I am in kernel now!\n");
    init_all();

	thread_start("k_thread_a", 32, k_thread_a, "argA");
    while(1);
    return 0;
}
