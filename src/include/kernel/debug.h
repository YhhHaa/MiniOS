#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H


void panic_spin(char* filename, int line, const char* func, const char* condition);
/* 
...: 表示可变参数
__FILE__: 被编译的文件名
__LINE__: 被编译文件中的行号
__func__: 被编译的函数名
__VA_ARGS__: 所有与可变参数对应的参数
 */
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) \
        if(CONDITION) {} else{  \
            PANIC(#CONDITION);  \
        }
#endif

#endif