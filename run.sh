#!/bin/sh

./build.sh && qemu-system-i386 -m 2048 -s -fda ./x86_tests/test.bin
