#!/bin/sh
set -e
clang -g -O3 -Wall -Wextra -o yuvbench.o -c yuvbench.c
clang -g -O3 -Wall -Wextra -o yuvbench_corevideo.o -c yuvbench_corevideo.c
clang -g -O3 -Wall -Wextra -o yuvbench-naive.o -c yuvbench_naive.c
clang -g -o yuvbench *.o -framework Accelerate