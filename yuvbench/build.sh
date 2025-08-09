#!/bin/sh
clang -g -O3 -Wall -Wextra -o yuvbench.o -c yuvbench.c
clang -g -o yuvbench *.o