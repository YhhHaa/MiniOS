#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"

/* inode结构 */
struct inode {
	uint32_t i_no; // inode编号
	uint32_t i_size; // 当此inode是文件时, i_size是指文件大小; 如果是目录, 指该目录下所有目录项大小之和

	uint32_t i_open_cnt; // 记录此文件被打开的次数
	bool write_deny; // 写文件不能并行, 进程写文件前查看此标识

	uint32_t i_sectors[13]; // 0~11是直接块, 12是一级间接指针块
	struct list_elem inode_tag; // 用于加入已打开的inode列表, 相当于内存缓冲
};
#endif