// ================================================================================================
// Code written while reading chapter 11 of Modern Arm Assembly Language Programming
//
// License:
//     SPDX-License-Identifier: 0BSD
//     Copyright (c) 2026 Hunter Kvalevog
//
//     Permission to use, copy, modify, and/or distribute this software for any
//     purpose with or without fee is hereby granted.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//     WITH REGARD TO THIS SOFTWARE.
// ================================================================================================

#include "common.S"

TEXT_SEGMENT

// example 1

.global SYM(i32_add_sub)
.global SYM(i64_add_sub)

SYM(i32_add_sub):
    // w0: a
    // w1: b
    // w2: c
#if 0
    add w3, w0, w1
    sub w0, w3, w2
#else
    add w0, w0, w1
    sub w0, w0, w2
#endif
    // w0: return
    ret

SYM(i64_add_sub):
    // x0: a
    // x1: b
    // x2: c
    add x3, x0, x1
    sub x0, x3, x2
    // x0: return
    ret

// example 2

.global SYM(i32_mul)
.global SYM(i32_mul_safe)

SYM(i32_mul):
    // w0: a
    // w1: b
    mul w0, w0, w1
    // w0: return
    ret

SYM(i32_mul_safe):
    // w0: a
    // w1: b
    smull x0, w0, w1
    // x0: ret
    ret

