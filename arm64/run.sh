#!/bin/sh
D="$(dirname "$0")"
clang -o "$D/out" "$1" || exit 1
lldb --no-lldbinit -o "target create out" -o "process launch"
