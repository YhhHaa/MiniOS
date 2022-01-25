#include "../../include/kernel/list.h"


// 初始化双向链表list
void list_init(struct list* list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

// 将链表元素elem插入在元素before之前
void list_insert_before(struct list_elem* before, struct list_elem* elem) {
	// 关中断保证原子操作
	enum intr_status old_status = intr_disable();

	before->prev->next = elem;
	elem->next = before;
	elem->prev = before->prev;
	before->prev = elem;

	intr_set_status(old_status);
}

// 添加元素到列表队首, 类似于栈的push操作
void list_push(struct list* plist, struct list_elem* elem) {
	list_insert_before(plist->head.next, elem);
}

// 添加元素到链表队尾, 类似于队列的先进先出
void list_append(struct list* plist, struct list_elem* elem) {
	list_insert_before(&plist->tail, elem);
}

// 使元素pelem离开链表
void list_remove(struct list_elem* elem) {
	enum intr_status old_status = intr_disable();

	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;

	intr_set_status(old_status);
}

// 将链表第一个元素弹出并返回, 类似栈的pop操作
struct list_elem* list_pop(struct list* plist) {
	struct list_elem* elem = plist->head.next;
	list_remove(elem);
	return elem;
}

// 从链表中查找元素obj_elem, 成功时返回true, 失败时返回false
bool elem_find(struct list* plist, struct list_elem* obj_elem) {
	struct list_elem* elem = plist->head.next;
	while(elem != &plist->tail) {
		if(elem == obj_elem) {
			return true;
		}
		elem = elem->next;
	}
	return false;
}

// 把列表中的每个元素elem和arg传给回调函数func
struct list_elem* list_traversal(struct list* plist, function func, int arg) {
	struct list_elem* elem = plist->head.next;

	if(list_empty(plist)) {
		return NULL;
	}

	while(elem != &plist->tail) {
		if(func(elem, arg)) {
			return elem;
		}
		elem = elem->next;
	}
	return NULL;
}

// 返回链表长度
uint32_t list_len(struct list* plist) {
	struct list_elem* elem = plist->head.next;
	uint32_t length = 0;
	while(elem != &plist->tail) {
		length++;
		elem = elem->next;
	}
	return length;
}

// 判断链表是否为空
bool list_empty(struct list* plist) {
	return (plist->head.next == &plist->tail?true:false);
}
