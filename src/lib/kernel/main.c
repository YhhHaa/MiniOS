#include "../../include/kernel/main.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int prog_a_pid = 0, prog_b_pid = 0;

int main(void) {
    put_str("I am in kernel now!\n");
    init_all();

	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");

	intr_enable(); // 打开中断, 使得时钟中断起作用

	console_put_str("main_pid:0x");
	console_put_int(sys_getpid());
	console_put_char('\n');

	thread_start("consumer_a", 31, k_thread_a, " A_");
	thread_start("consumer_b", 31, k_thread_b, " B_");

    while(1);
    return 0;
}


void k_thread_a(void* arg) {
	char* para = arg;

	console_put_str("thread_a_pid:0x");
	console_put_int(sys_getpid());
	console_put_char('\n');

	console_put_str("prog_a_pid:0x");
	console_put_int(prog_a_pid);
	console_put_char('\n');

	while(1);
}

void k_thread_b(void* arg) {
	char* para = arg;

	console_put_str("thread_b_pid:0x");
	console_put_int(sys_getpid());
	console_put_char('\n');

	console_put_str("prog_b_pid:0x");
	console_put_int(prog_b_pid);
	console_put_char('\n');

	while(1);
}

void u_prog_a(void) {
	prog_a_pid = getpid();
	while(1);
}


void u_prog_b(void) {
	prog_b_pid = getpid();
	while(1);
	
}