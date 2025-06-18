nasm -I include -o ./temp/mbr.bin ./mbr/mbr.asm
dd if=./temp/mbr.bin of=c.img conv=notrunc
