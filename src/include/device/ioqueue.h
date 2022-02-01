#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H


#include "../kernel/stdint.h"
#include "../thread/thread.h"
#include "../thread/sync.h"
#include "../kernel/interrupt.h"
#include "../kernel/global.h"
#include "../kernel/debug.h"

#define bufsize 64

/* 环形队列 */
struct ioqueue {
	struct lock lock; /* 锁, 保证缓冲区操作互斥 */

	/* 生产者, 缓冲区不满时往里面放数据
	否则就睡眠, 此记录哪个生产者在此缓冲区上睡眠 */
	struct task_struct* producer; 

	/* 消费者, 缓冲区不空时继续从里面拿数据
	否则就睡眠, 此记录哪个消费者在此缓冲区上睡眠 */
	struct task_struct* consumer;

	char buf[bufsize]; // 缓冲区
	int32_t head; // 队首, 数据往队首处写入
	int32_t tail; // 队尾, 数据往队尾读出
};

void ioqueue_init(struct ioqueue* ioq);
static int32_t next_pos(int32_t pos);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
static void ioq_wait(struct task_struct** waiter);
static void wakeup(struct task_struct** waiter);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);


#endif