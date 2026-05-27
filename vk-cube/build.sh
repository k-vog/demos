#!/bin/bash
mkdir -p bin
cc -o bin/vk-cube -Wall -Wextra -Wpedantic -O0 -g ./vk-cube.c \
    $(pkg-config --cflags --libs sdl3 vulkan)
glslc -o bin/vk-cube-vs.spv -fshader-stage=vertex   ./vk-cube-vs.glsl
glslc -o bin/vk-cube-fs.spv -fshader-stage=fragment ./vk-cube-fs.glsl
