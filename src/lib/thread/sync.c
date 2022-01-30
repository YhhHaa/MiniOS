#include "../../include/thread/sync.h"


// 初始化信号量
void sema_init(struct semaphore* psema, uint8_t value) {
	psema->value = value; // 信号量初值
	list_init(&psema->waiters); // 等待队列
}

// 初始化锁
void lock_init(struct lock* plock) {
	plock->holder = NULL; // 锁的持有者
	plock->holder_repeat_nr = 0; // 重复申请锁的次数
	sema_init(&plock->semaphore, 1); // 初始化信号量
}

// 信号量down操作
void sema_down(struct semaphore* psema) {
	// 关中断保证原子操作
	enum intr_status old_status = intr_disable();

	while(psema->value == 0) { // 在下次调度唤醒时, 确定锁是否被占用
		// debug
		ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
		if(elem_find(&psema->waiters, &running_thread()->general_tag)) { // 如果等待者中找到了
			PANIC("sema_down: thread blocked has been in waiters_list\n");
		}

		// 加入等待队列并阻塞自身
		list_append(&psema->waiters, &running_thread()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	psema->value--;
	ASSERT(psema->value == 0);

	// 恢复中断
	intr_set_status(old_status);
}

// 信号量up操作
void sema_up(struct semaphore* psema) {
	// 关闭中断保证原子操作
	enum intr_status old_status = intr_disable();

	ASSERT(psema->value == 0); // debug

	if(!list_empty(&psema->waiters)) { // 如果有阻塞的线程, 将阻塞的线程唤醒
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;

	ASSERT(psema->value == 1); // debug

	// 恢复中断
	intr_set_status(old_status);
}

// 获取锁
void lock_acquire(struct lock* plock) {
	if(plock->holder != running_thread()) { // 排除自己已经拥有锁但是未释放的情况
		sema_down(&plock->semaphore); // 信号量P原子操作
		plock->holder = running_thread(); // 设置锁的持有者
		ASSERT(plock->holder_repeat_nr == 0); // debug
		plock->holder_repeat_nr = 1;
	} else { // 拥有锁但是未释放
		plock->holder_repeat_nr++;
	}
}

// 释放锁, 当前线程是锁的持有者
void lock_release(struct lock* plock) {
	ASSERT(plock->holder == running_thread()); // debug
	if(plock->holder_repeat_nr > 1) {
		plock->holder_repeat_nr--;
		return;
	}
	ASSERT(plock->holder_repeat_nr == 1);

	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_up(&plock->semaphore); // 信号量V原子操作
}
