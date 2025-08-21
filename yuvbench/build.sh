#!/bin/sh
set -e

CFLAGS="-g -O3 -Wall -Wextra $(pkgconf --cflags libswscale)"
LFLAGS="-g $(pkgconf --libs libswscale libavutil)"

if [[ "$(uname)" == "Darwin" ]]; then
  LFLAGS="$LFLAGS -framework Accelerate"
  clang $CFLAGS -o yuvbench_corevideo.o -c yuvbench_corevideo.c
fi

if [[ "$(uname)" == MINGW* ]]; then
  LFLAGS="$LFLAGS $(pkgconf --libs vulkan)"
  clang $CFLAGS -o yuvbench_vulkan.o -c yuvbench_vulkan.c
fi

clang $CFLAGS -o yuvbench.o -c yuvbench.c
clang $CFLAGS -o yuvbench-naive.o -c yuvbench_naive.c
clang $CFLAGS -o yuvbench-swscale.o -c yuvbench_swscale.c
clang -o yuvbench *.o $LFLAGS
