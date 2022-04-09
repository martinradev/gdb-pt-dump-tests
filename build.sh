#!/bin/sh

nasm x86_tests/boot.asm -f bin -o x86_tests/boot.bin && \
gcc x86_tests/entry.c -Ix86_tests/ -O0 -fno-builtin -m32 -no-pie -fno-PIE -ffreestanding -nostdlib -c -o x86_tests/entry_tmp.o && \
ld x86_tests/entry_tmp.o -m elf_i386 -Tx86_tests/linker.ld -o x86_tests/entry.o && \
objcopy --only-section=.text --output-target binary x86_tests/entry.o x86_tests/entry.bin && \
objdump -Mintel,i386 -m i386 -b binary -D x86_tests/entry.bin > x86_tests/entry.d && \
dd if=/dev/urandom bs=1 count=1024 of=./x86_tests/padding.bin && \
cat x86_tests/boot.bin x86_tests/entry.bin x86_tests/padding.bin > x86_tests/test.bin
