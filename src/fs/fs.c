#include "fs.h"
#include "super_block.h"
#include "inode.h"
#include "dir.h"
#include "stdint.h"
#include "stdio-kernel.h"
#include "list.h"
#include "string.h"
#include "ide.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "file.h"

extern uint8_t channel_cnt; // 按硬盘数计算的通道数
extern struct ide_channel channels[2]; // 有两个ide通道
extern struct list partition_list; // 分区队列

struct partition* cur_part; // 默认情况下操作的是哪个分区

// 在分区链表中找到名为 part_name 的分区, 并将其指针赋值给 cur_part
static bool mount_partition(struct list_elem* pelem, int arg) {
    char* part_name = (char*)arg;
    struct partition* part = elem2entry(struct partition, part_tag, pelem);
    if (!strcmp(part->name, part_name)) {
        cur_part = part;
        struct disk* hd = cur_part->my_disk;
 
        // sb_buf 用来存储从硬盘上读入的超级块
        struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);
 
        // 在内存中创建分区 cur_part 的超级块
        cur_part->sb = (struct super_block*)sys_malloc(sizeof(struct super_block));
        if (cur_part->sb == NULL) {
            PANIC("alloc memory failed!");
        }
 
        // 读入超级块
        memset(sb_buf, 0, SECTOR_SIZE);
        ide_read(hd, cur_part->start_lba+1, sb_buf, 1);
 
        // 把 sb_buf 中超级块的信息复制到分区的超级块 sb 中
        memcpy(cur_part->sb, sb_buf, sizeof(struct super_block));
 
        // 将硬盘上的块位图读入到内存
        cur_part->block_bitmap.bits = (uint8_t*)sys_malloc(sb_buf->block_bitmap_sects*SECTOR_SIZE);
        if (cur_part->block_bitmap.bits == NULL) {
            PANIC("alloc memory failed!");
        }
        cur_part->block_bitmap.btmp_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;
        // 从硬盘上读入块位图到分区的 block_bitmap.bits
        ide_read(hd, sb_buf->block_bitmap_lba, cur_part->block_bitmap.bits, sb_buf->block_bitmap_sects);
 
        // 将硬盘上的 inode 位图读入到内存
        cur_part->inode_bitmap.bits = (uint8_t*)sys_malloc(sb_buf->inode_bitmap_sects*SECTOR_SIZE);
        if (cur_part->inode_bitmap.bits == NULL) {
            PANIC("alloc memory failed!");
        }
        cur_part->inode_bitmap.btmp_bytes_len = sb_buf->inode_bitmap_sects*SECTOR_SIZE;
        // 从硬盘上读入 inode 位图到分区的 inode_bitmap.bits
        ide_read(hd, sb_buf->inode_bitmap_lba, cur_part->inode_bitmap.bits, sb_buf->inode_bitmap_sects);
 
        list_init(&cur_part->open_inodes);
        printk("mount %s done!\n", part->name);
 
        return true; // 使 list_traversal 停止遍历
    }
    return false; // 使 list_traversal 继续遍历
}


/* 创建文件系统, 
Args:
	part: partition*类型, 待创建文件系统的分区  */
static void partition_format(struct partition* part) {
	/* 引导块, 超级块, inode位图, inode数组, 空闲块位图 的大小*/
	uint32_t boot_sector_sects = 1; // 引导块占用的扇区数
	uint32_t super_block_sects = 1; // 超级块占用的扇区数
	uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR); // i结点位图占用的扇区数, 最多4096个文件
	uint32_t inode_table_sects = DIV_ROUND_UP(((sizeof(struct inode) * MAX_FILES_PER_PART)), SECTOR_SIZE); // i结点数组占用的扇区数

	/* 简单处理块位图占据的扇区数 */
	uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
	uint32_t free_sects = part->sec_cnt - used_sects;
	uint32_t block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
	uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects; // 位图中位的长度, 可用块的数量
	block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

	/* 超级块初始化
		1. 写入超级块
		1. 文件系统类型
		2. 超级块位置以及大小
		3. 空闲块位图位置以及大小
		4. inode位图的位置以及大小
		5. inode数组的位置以及大小
		6. 空闲块起始地址
		7. 根目录起始地址
	*/
	struct super_block sb;
	sb.magic = 0x19590318;
	sb.sec_cnt = part->sec_cnt;
	sb.inode_cnt = MAX_FILES_PER_PART;
	sb.part_lba_base = part->start_lba;
	sb.block_bitmap_lba = sb.part_lba_base + 2; // 第0块是引导块, 第1块是超级块
	sb.block_bitmap_sects = block_bitmap_sects;
	sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_bitmap_sects = inode_bitmap_sects;
	sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
	sb.inode_table_sects = inode_table_sects;
	sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
	sb.root_inode_no = 0; // inode数组中的第0个inode留给了根目录
	sb.dir_entry_size = sizeof(struct dir_entry);

	/* 打印信息 */
    printk("%s info:\n", part->name);
    printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   data_start_lba:0x%x\n", sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt, sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

	struct disk* hd = part->my_disk;
	/* 1. 将超级块写入本分区的1扇区, 0扇区是OBR扇区 */
	ide_write(hd, part->start_lba + 1, &sb, 1);
	printk("    super_block_lba:0x%x\n", part->start_lba + 1);

	// 找出数据量最大的元信息, 用其尺寸做存储缓冲区, 栈中无法存下, 所在在堆中申请内存
	uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
	buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
	uint8_t* buf = (uint8_t*)sys_malloc(buf_size);

	/* 2. 将块位图初始化并写入sb.block_bitmap_lba */
	// 初始化块位图block_bitmapko
	buf[0] |= 0x01; // 第0个块预留给根目录, 位图先占位
	uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
	uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
	uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);

	// 		1. 先将位图最后1字节到其所在的扇区的结束全置1, 因为是向上取整, 所以最后一个扇区肯定有没有作用的位
	memset(&buf[block_bitmap_last_byte], 0xff, last_size);

	//		2. 再将上一步中覆盖的最后1字节内的有效位重新置0
	uint8_t bit_idx = 0;
	while (bit_idx <= block_bitmap_last_bit) {
		buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
	}
	ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

	/* 3. 将inode位图初始化并写入sb.inode_bitmap_lba */
	// 清空缓冲区
	memset(buf, 0, buf_size);
	buf[0] |= 0x1; // 第0个inode分给了根目录
	/* 由于inode_table中共4096个inode, 位图inode_bitmap正好占用1扇区,
	即inode_bitmap_sects等于1, 所以位图中的位全都代表inode_table中的inode,
	无需再向block_bitmap那样单独处理最后1扇区的剩余部分 */
	ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

	/* 4. 将inode数组初始化并写入sb.inode_table_lba */
	memset(buf, 0, buf_size);
	struct inode* i = (struct inode*)buf;
	i->i_size = sb.dir_entry_size * 2; // .和..
	i->i_no = 0; // 根目录占inode数组的第0个inode
	i->i_sectors[0] = sb.data_start_lba;
	ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

	/* 5. 将根目录写入sb.data_start_lba */
	memset(buf, 0, buf_size);
	struct dir_entry* p_de = (struct dir_entry*)buf;

	memcpy(p_de->filename, ".", 1);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	p_de++;

	memcpy(p_de->filename, "..", 2);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;

	// sb.data_start_lba已经分配给了根目录, 里面是根目录的目录项
	ide_write(hd, sb.data_start_lba, buf, 1);

	printk("    root_dir_lba:0x%x\n", sb.data_start_lba);
	printk("%s format done\n", part->name);
	sys_free(buf);
}

// 将最上层路径名称解析出来
static char* path_parse(char* pathname, char* name_store) {
    // 根目录不需要单独解析
    if (pathname[0] == '/') {
        // 路径中出现 1 个或多个连续的字符 '/', 将这些 '/' 跳过
        while (*(++pathname) == '/');
    }
 
    // 开始一般的路径解析
    while (*pathname != '/' && *pathname != 0) {
        *name_store++ = *pathname++;
    }
 
    if (pathname[0] == 0) {
        return NULL;
    }
 
    return pathname;
}
 
// 返回路径深度, 比如 /a/b/c, 深度为 3
int32_t path_depth_cnt(char* pathname) {
    ASSERT(pathname != NULL);
    char* p = pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;
 
    // 解析路径, 从中拆分出各级名称
    p = path_parse(p, name);
    while (name[0]) {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (p) {
            p = path_parse(p, name);
        }
    }
    return depth;
}

// 搜索文件 pathname, 若找到则返回其 inode 号, 否则返回 -1
static int search_file(const char* pathname, struct path_search_record* searched_record) {
    // 如果待查找的是根目录, 为避免下面无用的查找, 直接返回已知根目录信息
    if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") || \
        !strcmp(pathname, "/..")) {
        searched_record->parent_dir = &root_dir;
        searched_record->file_type = FT_DIRECTORY;
        searched_record->searched_path[0] = 0; // 搜索路径置空
        return 0;
    }
 
    uint32_t path_len = strlen(pathname);
    // 保证 pathname 至少是这样的路径 /x, 且小于最大长度
    ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
    char* sub_path = (char*)pathname;
    struct dir* parent_dir = &root_dir;
    struct dir_entry dir_e;
 
    // 记录路径解析出来的各级名称
    char name[MAX_FILE_NAME_LEN] = {0};
 
    searched_record->parent_dir = parent_dir;
    searched_record->file_type = FT_UNKNOWN;
    uint32_t parent_inode_no = 0; // 父目录的 inode 号
 
    sub_path = path_parse(sub_path, name);
    while (name[0]) { // 若第一个字符就是结束符, 结束循环
        // 记录查找过的路径, 但不能超过 searched_path 的长度 512 字节
        ASSERT(strlen(searched_record->searched_path) < 512);
 
        //  记录已存在的父目录
        strcat(searched_record->searched_path, "/");
        strcat(searched_record->searched_path, name);
 
        // 在所给的目录中查找文件
        if (search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
            memset(name, 0, MAX_FILE_NAME_LEN);
            // 若 sub_path 不等于 NULL, 也就是未结束时继续拆分路径
            if (sub_path) {
                sub_path = path_parse(sub_path, name);
            }
 
            // 如果被打开的是目录
            if (FT_DIRECTORY == dir_e.f_type) {
                parent_inode_no = parent_dir->inode->i_no;
                dir_close(parent_dir);
                parent_dir = dir_open(cur_part, dir_e.i_no); // 更新父目录
                searched_record->parent_dir = parent_dir;
                continue;
            } else if (FT_REGULAR == dir_e.f_type) { // 若是普通文件
                searched_record->file_type = FT_REGULAR;
                return dir_e.i_no;
            }
        } else { // 若找不到, 则返回 -1
            return -1;
        }
    }
 
    // 执行到此, 必然是遍历了完整路径并且查找的文件或目录只有同名目录存在
    dir_close(searched_record->parent_dir);
 
    // 保存被查找目录的直接父目录
    searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
    searched_record->file_type = FT_DIRECTORY;
    return dir_e.i_no;
}

// 打开或创建文件成功后, 返回文件描述符, 否则返回 -1
int32_t sys_open(const char* pathname, uint8_t flags) {
    // 对目录要用 dir_open, 这里只有 open 文件
    if (pathname[strlen(pathname) - 1] == '/') {
        printk("can`t open a directory %s\n", pathname);
        return -1;
    }
    ASSERT(flags <= 7);
    int32_t fd = -1; // 默认为找不到
	
	// 用于判断父目录以及失败的情况
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
 
    // 记录目录深度, 帮助判断中间某个目录不存在的情况
    uint32_t pathname_depth = path_depth_cnt((char*)pathname);
    
    // 先检查文件是否存在
    int inode_no = search_file(pathname, &searched_record);
    bool found = inode_no != -1 ? true : false;
	
	/* *************************************** */
	// 失败情况1: 最后一项是目录-> -1
	/* *************************************** */
    if (searched_record.file_type == FT_DIRECTORY) {
        printk("can`t open a directory with open(), use opendir() to instead\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }
	/* *************************************** */
	/* 失败情况2: 中间一项是文件或中间一项不存在-> -1 */
	/* *************************************** */
    uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);
    // 先判断是否把 pathname 的各层目录都访问到了, 即是否在某个中间目录就失败了
    if (pathname_depth != path_searched_depth) {
        printk("cannot access %s: Not a directory, subpath %s is`t exist\n", pathname, searched_record.searched_path);
        dir_close(searched_record.parent_dir);
        return -1;
    }
	
	/* ********************************************** */
	/* 失败情况3: 最后一项找不到且该函数标志不是创建文件-> -1 */
	/* ********************************************** */
    if (!found && !(flags & O_CREAT)) {
        printk("in path %s, file %s is`t exist\n", searched_record.searched_path, (strrchr(searched_record.searched_path, '/') + 1));
        dir_close(searched_record.parent_dir);
        return -1;
    } else if (found && flags & O_CREAT) { // 若要创建的文件已存在
        printk("%s has already exist!\n", pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }
 
    switch (flags & O_CREAT) {
        case O_CREAT:
            printk("creating file\n");
            fd = file_create(searched_record.parent_dir, (strrchr(pathname, '/') + 1), flags);
            dir_close(searched_record.parent_dir);
            break;
        // 其余为打开文件
		default:
			fd = file_open(inode_no, flags);
    }
 
    // 此 fd 是指任务 pcb->fd_table 数组中的元素下标
    // 并不是指全局 file_table 中的下标
    return fd;
}

/* 在磁盘上搜索文件系统,若没有则格式化分区创建文件系统 */
void filesys_init() {
	uint8_t channel_no = 0, dev_no, part_idx = 0;

    /* sb_buf用来存储从硬盘上读入的超级块 */
    struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);

    if (sb_buf == NULL) {
        PANIC("alloc memory failed!");
    }
    printk("searching filesystem......\n");
    while (channel_no < channel_cnt) {
        dev_no = 0;
        while(dev_no < 2) {
            if (dev_no == 0) {   // 跨过裸盘hd60M.img
                dev_no++;
                continue;
            }
            struct disk* hd = &channels[channel_no].devices[dev_no];
            struct partition* part = hd->prim_parts;
            while(part_idx < 12) {   // 4个主分区+8个逻辑
                if (part_idx == 4) {  // 开始处理逻辑分区
                    part = hd->logic_parts;
                }

                /* channels数组是全局变量,默认值为0,disk属于其嵌套结构,
                 * partition又为disk的嵌套结构,因此partition中的成员默认也为0.
                 * 若partition未初始化,则partition中的成员仍为0. 
                 * 下面处理存在的分区. */
                if (part->sec_cnt != 0) {  // 如果分区存在
                    memset(sb_buf, 0, SECTOR_SIZE);

                    /* 读出分区的超级块,根据魔数是否正确来判断是否存在文件系统 */
                    ide_read(hd, part->start_lba + 1, sb_buf, 1);   

                    /* 只支持自己的文件系统.若磁盘上已经有文件系统就不再格式化了 */
                    if (sb_buf->magic == 0x19590318) {
                        printk("%s has filesystem\n", part->name);
                    } else {			  // 其它文件系统不支持,一律按无文件系统处理
                        printk("formatting %s`s partition %s......\n", hd->name, part->name);
                        partition_format(part);
                    }
                }
                part_idx++;
                part++;	// 下一分区
            }
            dev_no++;	// 下一磁盘
        }
        channel_no++;	// 下一通道
    }
    sys_free(sb_buf);

    // 确定默认操作的分区
    char default_part[8] = "sdb1";
    list_traversal(&partition_list, mount_partition, (int)default_part);

	/* 打开根目录 */
	open_root_dir(cur_part);

	/* 初始化文件表 */
	uint32_t fd_idx = 0;
	while (fd_idx < MAX_FILE_OPEN) {
		file_table[fd_idx++].fd_inode = NULL;
	}
}