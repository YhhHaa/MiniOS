# mbr
nasm -I ./include -o ./bin/mbr.bin ./lib/mbr.S
dd if=./bin/mbr.bin of=../bochs/hd60M.img bs=512 count=1 conv=notrunc

# loader
nasm -I ./include -o ./bin/loader.bin ./lib/loader.S
dd if=./bin/loader.bin of=../bochs/hd60M.img bs=512 count=5 seek=2 conv=notrunc

# kernel
#   main.c
gcc -I ./include -m32 -c -fno-builtin -o ./temp/main.o ./lib/kernel/main.c 
#   pirnt.S
nasm -f elf -I ./include -o ./temp/print.o ./lib/kernel/print.S
#   kernel.S
nasm -f elf -I ./include -o ./temp/kernel.o ./lib/kernel/kernel.S
#   interrupt.c
gcc -I ./include -m32 -c -fno-builtin -o ./temp/interrupt.o ./lib/kernel/interrupt.c
#   init.c
gcc -I ./include -m32 -c -fno-builtin -o ./temp/init.o ./lib/kernel/init.c
#   connect
ld -m elf_i386 -Ttext 0xc0001500 -e main -o ./bin/kernel.bin ./temp/main.o ./temp/init.o  \
    ./temp/interrupt.o ./temp/print.o ./temp/kernel.o

#   write into kernel
dd if=./bin/kernel.bin of=../bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
