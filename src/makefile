BUILD_DIR = ./build
BIN_DIR = ./bin
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I ./include
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -m32 -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLASG = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o  \
    $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o \
	$(BUILD_DIR)/string.o


# C代码编译
$(BUILD_DIR)/main.o: lib/kernel/main.c include/kernel/print.h include/kernel/stdint.h include/kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: lib/kernel/init.c include/kernel/init.h include/kernel/print.h include/kernel/stdint.h include/kernel/interrupt.h include/device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: lib/kernel/interrupt.c include/kernel/interrupt.h include/kernel/stdint.h include/kernel/global.h include/kernel/io.h include/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: lib/device/timer.c include/device/timer.h include/kernel/stdint.h include/kernel/io.h include/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: lib/kernel/debug.c include/kernel/debug.h include/kernel/print.h include/kernel/stdint.h include/kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: lib/kernel/memory.c include/kernel/memory.h include/kernel/stdint.h include/kernel/print.h include/kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c include/kernel/bitmap.h include/kernel/stdint.h include/string/string.h include/kernel/print.h include/kernel/interrupt.h include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string/string.c include/string/string.h include/kernel/global.h include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@


# 汇编代码编译
$(BUILD_DIR)/kernel.o: lib/kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

$(BIN_DIR)/mbr.bin: lib/mbr.S
	$(AS) $< -o $@

$(BIN_DIR)/loader.bin: lib/loader.S
	$(AS) $< -o $@

# 链接所有目标文件
$(BIN_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLASG) $^ -o $@

.PHONY: mk_dir hd clean all run

mk_dir: 
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

hd:
	dd if=$(BIN_DIR)/mbr.bin of=../bochs/hd60M.img bs=512 count=0 conv=notrunc
	dd if=$(BIN_DIR)/loader.bin of=../bochs/hd60M.img bs=512 count=5 seek=2 conv=notrunc
	dd if=$(BIN_DIR)/kernel.bin of=../bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

clean:
	rm -f $(BUILD_DIR)/*
	rm -f $(BIN_DIR)/*

build: $(BIN_DIR)/kernel.bin $(BIN_DIR)/mbr.bin $(BIN_DIR)/loader.bin

run:
	../bochs/bin/bochs -f ../bochs/bochsrc.disk

all: mk_dir build hd run

# # mbr
# nasm -I ./include -o ./bin/mbr.bin ./lib/mbr.S
# dd if=./bin/mbr.bin of=../bochs/hd60M.img bs=512 count=1 conv=notrunc

# # loader
# nasm -I ./include -o ./bin/loader.bin ./lib/loader.S
# dd if=./bin/loader.bin of=../bochs/hd60M.img bs=512 count=5 seek=2 conv=notrunc

# # kernel
# #   main.c
# gcc -I ./include -m32 -c -fno-builtin -o ./temp/main.o ./lib/kernel/main.c 
# #   pirnt.S
# nasm -f elf -I ./include -o ./temp/print.o ./lib/kernel/print.S
# #   kernel.S
# nasm -f elf -I ./include -o ./temp/kernel.o ./lib/kernel/kernel.S
# #   interrupt.c
# gcc -I ./include -m32 -c -fno-builtin -o ./temp/interrupt.o ./lib/kernel/interrupt.c
# #   init.c
# gcc -I ./include -m32 -c -fno-builtin -o ./temp/init.o ./lib/kernel/init.c
# #   timer.c
# gcc -I ./include -m32 -c -o ./temp/timer.o ./lib/device/timer.c
# #   connect
# ld -m elf_i386 -Ttext 0xc0001500 -e main -o ./bin/kernel.bin ./temp/main.o ./temp/init.o  \
#     ./temp/interrupt.o ./temp/print.o ./temp/kernel.o ./temp/timer.o

# #   write into kernel
# dd if=./bin/kernel.bin of=../bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
