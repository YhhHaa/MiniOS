#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "sync.h"
#include "bitmap.h"

/* 分区结构 */
struct partition {
	uint32_t start_lba; // 起始扇区
	uint32_t sec_cnt; // 扇区数
	struct disk* my_disk; // 分区所属的硬盘
	struct list_elem part_tag; // 用于队列中的标记
	char name[8]; // 分区名称, 如sd1, sda2
	struct super_block* sb; // 本分区的超级块
	// 文件系统中涉及的
	struct bitmap block_bitmap; // 块位图
	struct bitmap inode_bitmap; // i结点位图
	struct list open_inodes; // 本分区打开的i结点队列
};

/* 硬盘结构 */
struct disk {
	char name[8]; // 本硬盘的名称, 如sda, sdb
	struct ide_channel* my_channel; // 此块硬盘归属于哪个ide通道
	uint8_t dev_no; // 本硬盘是主0还是从1
	struct partition prim_parts[4]; // 主分区顶多是4个
	struct partition logic_parts[8]; // 逻辑分区支持8个
};

/* ata通道结构 */
struct ide_channel {
	char name[8]; // 本ata通道名称, 如ata0, ide0
	uint16_t port_base; // 本通道的起始端口号,通道1和通道2端口号不同
	uint8_t irq_no; // 本通道所用的中断号, 根据中断号判断是哪个通道
	struct lock lock; // 通道锁, 用来发送中断时只允许一个通道发送, 防止混淆
	bool expecting_intr; // 表示本通道正在等待硬盘中断
	struct semaphore disk_done; // 发送完中断, 用于阻塞, 唤醒自身, 防止浪费CPU
	struct disk devices[2]; // 一个通道上连接两个硬盘, 一主一丛
};

void ide_init(void);
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void intr_hd_handler(uint8_t irq_no);
#endif