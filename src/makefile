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
	$(BUILD_DIR)/thread.o $(BUILD_DIR)/string.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/list.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o \
	$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o \
	$(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/syscall.o


# C代码编译
$(BUILD_DIR)/main.o: lib/kernel/main.c include/kernel/main.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: lib/kernel/init.c include/kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: lib/kernel/interrupt.c include/kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: lib/device/timer.c include/device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: lib/kernel/debug.c include/kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: lib/kernel/memory.c include/kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c include/kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: lib/thread/thread.c include/thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string/string.c include/string/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: lib/kernel/list.c include/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o: lib/thread/sync.c include/thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: lib/device/console.c include/device/console.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: lib/device/keyboard.c include/device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o: lib/device/ioqueue.c include/device/ioqueue.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o: lib/userprog/tss.c include/userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: lib/userprog/process.c include/userprog/process.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: lib/user/syscall.c include/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o: lib/userprog/syscall-init.c include/userprog/syscall-init.h
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

$(BUILD_DIR)/switch.o: lib/thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@


# 链接所有目标文件
$(BIN_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLASG) $^ -o $@

.PHONY: mk_dir hd clean all run

mk_dir: 
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	if [ ! -d $(BIN_DIR) ]; then mkdir $(BIN_DIR); fi

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

