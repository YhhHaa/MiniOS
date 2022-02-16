gcc -m32 -Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes \
  -Wsystem-headers -I ../lib -o prog_no_arg.o prog_no_arg.c
ld -melf_i386 -e main prog_no_arg.o ../build/string.o ../build/syscall.o\
  ../build/stdio.o ../build/assert.o -o prog_no_arg
dd if=prog_no_arg of=../hd3M.img bs=512 count=10 seek=300 conv=notrunc
