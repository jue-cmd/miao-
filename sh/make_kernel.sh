nasm -I include -f elf32 -o ./temp/print.o ./kernel/print.asm
nasm -I include -f elf32 -o ./temp/kernel.o ./kernel/kernel.asm
gcc -m32 -fno-stack-protector -c -o ./temp/main.o ./kernel/main.c 
gcc -m32 -fno-stack-protector -c -o ./temp/strings.o ./kernel/strings.c
gcc -m32 -fno-stack-protector -c -o ./temp/interrupt.o ./kernel/interrupt.c
gcc -m32 -fno-stack-protector -c -o ./temp/init.o ./kernel/init.c
gcc -m32 -fno-stack-protector -c -o ./temp/timer.o ./kernel/device/timer.c
gcc -m32 -fno-stack-protector -c -o ./temp/debug.o ./kernel/debug.c

ld -m elf_i386 ./temp/main.o ./temp/print.o ./temp/strings.o ./temp/interrupt.o  ./temp/init.o ./temp/kernel.o ./temp/timer.o ./temp/debug.o -Ttext 0xc0001500 -e main -o ./temp/kernel.bin 
dd if=./temp/kernel.bin of=./c.img bs=512 count=200 seek=9 conv=notrunc