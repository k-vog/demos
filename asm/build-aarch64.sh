#!/bin/sh
set -e
clang -O0 -g -fsanitize=address -Wall -Wextra -Wpedantic -Wno-unused-parameter -c -o aarch64_1.o aarch64.c
clang -O0 -g -c -o aarch64_2.o aarch64.s
clang -O0 -g -fsanitize=address -o aarch64 aarch64_1.o aarch64_2.o
