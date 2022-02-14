#ifndef __FS_FS_H
#define __FS_FS_H

#include "stdint.h"

#define MAX_FILES_PER_PART 4096 // 每个分区支持最大创建的文件数
#define BITS_PER_SECTOR 4096 // 每个扇区的位数
#define SECTOR_SIZE 512 // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE // 块字节大小

#define MAX_PATH_LEN 512 // 路径最大长度

/* 文件类型 */
enum file_types {
	FT_UNKNOWN, // 不支持的文件类型
	FT_REGULAR, // 普通文件
	FT_DIRECTORY // 目录文件
};

/* 打开文件的选项 */
enum oflags { // 按位来表示
	O_RDONLY, // 0x00只读
	O_WRONLY, // 0x01只写
	O_RDWR, // 0x10读写
	O_CREAT = 4 // 0x100创建
};

/* 文件读写位置偏移量 */
enum whence {
	SEEK_SET = 1, // offset参照物是文件开始处
	SEEK_CUR, // offset参照是文件当前读写位置
	SEEK_END // offset参照物是文件尺寸大小
};
 
// 用来记录查找文件过程中已找到的上级路径
struct path_search_record {
    char searched_path[MAX_PATH_LEN]; // 查找过程中的父路径
    struct dir* parent_dir; // 文件或目录所在的直接父目录
    enum file_types file_type; // 文件类型
};

extern struct partition* cur_part; // 默认情况下操作的是哪个分区
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char* pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);
int32_t sys_read(int32_t fd, void* buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
void filesys_init(void);

#endif