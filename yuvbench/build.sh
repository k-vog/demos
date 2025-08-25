#!/bin/sh
set -e

CFLAGS="-g -O3 -Wall -Wextra $(pkgconf --cflags libswscale vulkan)"
LFLAGS="-g $(pkgconf --libs libswscale libavutil vulkan)"

if [[ "$(uname)" == "Darwin" ]]; then
  LFLAGS="$LFLAGS -framework Accelerate"
  clang $CFLAGS -o yuvbench_corevideo.o -c yuvbench_corevideo.c
fi

clang $CFLAGS -o yuvbench.o -c yuvbench.c
clang $CFLAGS -o yuvbench-naive.o -c yuvbench_naive.c
clang $CFLAGS -o yuvbench-swscale.o -c yuvbench_swscale.c
clang $CFLAGS -o yuvbench_vulkan.o -c yuvbench_vulkan.c
clang -o yuvbench *.o $LFLAGS
