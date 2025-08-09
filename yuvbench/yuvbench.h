#ifndef _YUVBENCH_H_
#define _YUVBENCH_H_

#include <stddef.h>
#include <stdint.h>

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef int32_t   i32;

#define r_assert(expr)                                                        \
  do {                                                                        \
    if (!(expr)) {                                                            \
      printf("%s:%d: Assertion failed:\n\t%s\n", __FILE__, __LINE__, #expr);  \
      __builtin_trap();                                                       \
      abort();                                                                \
    }                                                                         \
  } while (0);

typedef struct Context Context;
struct Context
{
  // Image dimensions
  u32     width;
  u32     height;
  u32     uv_width;
  u32     uv_height;
  // Out buffer - RGBA32 interleaved
  u8*     out;
  size_t  out_len;
  i32     out_stride;
  // Inp buffer - Y plane
  u8*     inp_y;
  size_t  inp_y_len;
  i32     inp_y_stride;
  // Inp buffer - U plane
  u8*     inp_u;
  size_t  inp_u_len;
  i32     inp_u_stride;
  // Inp buffer - V plane
  u8*     inp_v;
  size_t  inp_v_len;
  i32     inp_v_stride;
};

#endif // _YUVBENCH_H_