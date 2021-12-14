# mbr
nasm -I ./include -o ./bin/mbr.bin ./lib/mbr.S
dd if=./bin/mbr.bin of=../bochs/hd60M.img bs=512 count=1 conv=notrunc

# loader
nasm -I ./include -o ./bin/loader.bin ./lib/loader.S
dd if=./bin/loader.bin of=../bochs/hd60M.img bs=512 count=5 seek=2 conv=notrunc

# kernel
#   pirnt.S
nasm -f elf -I ./include -o ./temp/print.o ./lib/kernel/print.S

#   main.c
gcc -I ./include -m32 -c -o ./temp/main.o ./lib/kernel/main.c 
ld -m elf_i386 -Ttext 0xc0001500 -e main -o ./bin/kernel.bin ./temp/main.o ./temp/print.o

#   write into kernel
dd if=./bin/kernel.bin of=../bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
