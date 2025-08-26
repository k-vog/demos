#ifndef _YUVBENCH_H_
#define _YUVBENCH_H_

#include "com_base.hh"

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
  usize   out_len;
  i32     out_stride;
  // Input buffer - Y plane
  u8*     inp_y;
  usize   inp_y_len;
  i32     inp_y_stride;
  // Input buffer - U plane
  u8*     inp_u;
  usize   inp_u_len;
  i32     inp_u_stride;
  // Input buffer - V plane
  u8*     inp_v;
  usize   inp_v_len;
  i32     inp_v_stride;
  // Implementation
  void*   impl;
};

void CoreVideo_Create(Context* ctx);
void CoreVideo_Process(Context* ctx);
void CoreVideo_Destroy(Context* ctx);

void Naive_Create(Context* ctx);
void Naive_Process(Context* ctx);
void Naive_Destroy(Context* ctx);

void Swscale_Create(Context* ctx);
void Swscale_Process(Context* ctx);
void Swscale_Destroy(Context* ctx);

void Vulkan_Create(Context* ctx);
void Vulkan_Process(Context* ctx);
void Vulkan_Destroy(Context* ctx);

#endif // _YUVBENCH_H_
