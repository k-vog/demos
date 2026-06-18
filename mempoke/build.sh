#!/bin/bash
mkdir -p bin
cc -o bin/mempoke-target   -Wall -Wextra -Wpedantic -O0 -g ./mempoke-target.c
cc -o bin/mempoke-injector -Wall -Wextra -Wpedantic -O0 -g ./mempoke-injector.c
