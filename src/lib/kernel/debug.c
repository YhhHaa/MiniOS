#include "../../include/kernel/debug.h"
#include "../../include/kernel/print.h"
#include "../../include/kernel/interrupt.h"


void panic_spin(char* filename, int line, const char* func, const char* condition) {
    intr_disable(); // 关闭中断
    // 打印错误信息
    put_str("\n\n\n!!!!error!!!!\n");
    put_str("filename:");put_str(filename);put_str("\n");
    put_str("line:0x:");put_int(line);put_str("\n");
    put_str("func:");put_str(func);put_str("\n");
    put_str("condition:");put_str(condition);put_str("\n");
    while(1);
}