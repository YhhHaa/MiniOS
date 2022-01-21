#include "../../include/kernel/print.h"
#include "../../include/kernel/init.h"
#include "../../include/kernel/debug.h"
#include "../../include/kernel/memory.h"


void main(void) {
    put_str("I am in kernel now!\n");
    init_all();

	// ASSERT(0);
	void* addr = get_kernel_pages(3);
	put_str("\n get_kernel_page start vaddr is ");
	put_int((uint32_t)addr);
	put_str("\n");
    while(1);
    return 0;
}
