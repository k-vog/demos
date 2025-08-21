#ifndef _YUVBENCH_H_
#define _YUVBENCH_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int32_t   i32;
typedef float     f32;
typedef double    f64;

#pragma clang diagnostic ignored "-Wunused-parameter"

#define r_assert(expr)                                                        \
  do {                                                                        \
    if (!(expr)) {                                                            \
      printf("%s:%d: Assertion failed:\n\t%s\n", __FILE__, __LINE__, #expr);  \
      __builtin_trap();                                                       \
      abort();                                                                \
    }                                                                         \
  } while (0);

static inline u32 next_multiple(u32 x, u32 m)
{
  return ((x + m - 1) / m) * m;
}

typedef struct Context Context;
struct Context
{
  // Image dimensions
  u32     width;
  u32     height;
  u32     uv_width;
  u32     uv_height;
  // Image alignment
  u32     alignment;
  // Out buffer - RGBA32 interleaved
  u8*     out;
  size_t  out_len;
  i32     out_stride;
  // Input buffer - Y plane
  u8*     inp_y;
  size_t  inp_y_len;
  i32     inp_y_stride;
  // Input buffer - U plane
  u8*     inp_u;
  size_t  inp_u_len;
  i32     inp_u_stride;
  // Input buffer - V plane
  u8*     inp_v;
  size_t  inp_v_len;
  i32     inp_v_stride;
  // Implementation
  void*   impl;
};

void yuv_corevideo_create(Context* ctx);
void yuv_corevideo_process(Context* ctx);
void yuv_corevideo_destroy(Context* ctx);

void yuv_naive_create(Context* ctx);
void yuv_naive_process(Context* ctx);
void yuv_naive_destroy(Context* ctx);

void yuv_swscale_create(Context* ctx);
void yuv_swscale_process(Context* ctx);
void yuv_swscale_destroy(Context* ctx);

void yuv_vulkan_create(Context* ctx);
void yuv_vulkan_process(Context* ctx);
void yuv_vulkan_destroy(Context* ctx);

#endif // _YUVBENCH_H_
