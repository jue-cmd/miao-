nasm -I include -o ./temp/loader.bin ./mbr/loader.asm
dd if=./temp/loader.bin of=c.img seek=2  conv=notrunc