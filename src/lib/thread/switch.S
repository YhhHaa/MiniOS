[bits 32]
section .text
global switch_to
switch_to:
	;备份当前线程cur的寄存器映像环境
	push esi
	push edi
	push ebx
	push ebp

	mov eax, [esp + 20] ;得到栈中的参数cur
	mov [eax], esp ;保存栈顶指针esp, task_struct的self_kstack字段, 偏移量为0

	;恢复下一个线程的环境
	mov eax, [esp + 24] ;得到栈中参数next
	mov esp, [eax] ;PCB的第一个成员是self_kstack成员, 记录0级栈顶指针
	pop ebp
	pop ebx
	pop edi
	pop esi
	;进入栈中的返回地址
	;若是第一次执行, self_kstack栈顶内容为kernel_thread函数地址
	;若以前执行过, self_kstack栈顶内容为switch_to的主调函数schedule留下的, 会压入参数, 返回地址
	;继续执行schedule, 而switch_to是schedule最后一句, 因此执行流程会回到schedule调用者
	;intr_timer_handler中, 而schedule也是intr_timer_handler最后一句, 因此会完成intr_timer_handler
	;程序返回kernel.S的jmp intr_exit恢复任务的全部寄存器映像, 通过iretd指令退出中断
	ret
