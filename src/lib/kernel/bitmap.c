#include "../../include/kernel/bitmap.h"


// 初始化位图长度以及内容
void bitmap_init(struct bitmap* btmp) {
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

// 判断bit_idx位是否为1
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
	uint32_t byte_idx = bit_idx / 8; // 索引数组下标
	uint32_t bit_odd = bit_idx % 8; // 取余索引在一个字节内的位置
	return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

// 在位图中申请连续cnt个位, 成功返回其起始下标, 失败返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
	uint32_t idx_byte = 0; // 记录第一个空闲位所在字节
	while(0xff == btmp->bits[idx_byte] && (idx_byte < btmp->btmp_bytes_len)) {
		idx_byte++;
	}

	// 没有空闲位
	ASSERT(idx_byte < btmp->btmp_bytes_len);
	if(idx_byte == btmp->btmp_bytes_len) {
		return -1;
	}
	
	// 有空闲位
	int idx_bit = 0; // 记录此字节内第一个空闲位的下标
	while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) {
		idx_bit++;
	}
	int bit_idx_start = idx_byte * 8 + idx_bit;  // 整个位图的索引
	if(cnt == 1) {
		return bit_idx_start;
	}

	uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);
	uint32_t next_bit = bit_idx_start + 1;
	uint32_t count = 1;

	bit_idx_start = -1;
	while(bit_left-- > 0) {
		if(!(bitmap_scan_test(btmp, next_bit))) {
			count++;
		} else {
			count = 0;
		}
		if(count == cnt) {
			bit_idx_start = next_bit - cnt + 1;
			break;
		}
		next_bit++;
	}
	return bit_idx_start;
}

// 将位图btmp的bit_idx位设置为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
	ASSERT((value == 0) || (value == 1));
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;

	if(value) {
		btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
	} else {
		btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
	}
}
