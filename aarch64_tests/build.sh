#!/bin/sh

aarch64-linux-gnu-gcc -ggdb3 -std=gnu99 -Wall -c -o main.o main.c &&
aarch64-linux-gnu-as -o startup.o startup.asm &&
aarch64-linux-gnu-ld -Bstatic --gc-sections -nostartfiles -nostdlib -o first-steps.elf -T link.ld main.o
