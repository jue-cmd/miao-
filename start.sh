#!/bin/bash

qemu-system-i386 \
  -m 256 \
  -vga std \
  -display sdl \
  -D bochs.out \
  -drive file=./c.img,format=raw,media=disk \
  -boot order=c -d int,cpu_reset -D qemu.log