.section __TEXT,__text
.global _addi32
.global _subi32

.global _addf32
.global _subf32

.global _addf32v4_s
.global _addf32v4_v

_addi32:
  add w0, w0, w1
  ret

_subi32:
  sub w0, w0, w1
  ret

_addf32:
  fadd s0, s0, s1
  ret

_subf32:
  fsub s0, s0, s1
  ret

// x0: f32[4]
// x1: f32[4]
// x2: f32[4]
_addf32v4_s:
  ldr s0, [x1, #0 ]
  ldr s1, [x1, #4 ]
  ldr s2, [x1, #8 ]
  ldr s3, [x1, #12]

  ldr s4, [x2, #0 ]
  ldr s5, [x2, #4 ]
  ldr s6, [x2, #8 ]
  ldr s7, [x2, #12]

  fadd s8,  s0, s4
  fadd s9,  s1, s5
  fadd s10, s2, s6
  fadd s11, s3, s7

  str s8,  [x0, #0 ]
  str s9,  [x0, #4 ]
  str s10, [x0, #8 ]
  str s11, [x0, #12]

  ret

// x0: f32[4]
// x1: f32[4]
// x2: f32[4]
_addf32v4_v:
  ldr q0, [x1]
  ldr q1, [x2]

  fadd v2.4s, v0.4s, v1.4s

  str q2, [x0]

  ret

