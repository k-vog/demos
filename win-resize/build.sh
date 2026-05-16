#!/bin/bash
mkdir -p bin
cc -o bin/win-resize -Wall -Wextra -Wpedantic -O0 -g ./win-resize.c -ldwmapi -lgdi32
