#include "yuvbench.hh"

void Naive_Create(Context* ctx)
{
}

void Naive_Process(Context* ctx)
{
  for (u32 y = 0; y < ctx->height; ++y) {
    u32 uvy = y / 2;
    u8* row = ctx->out + (ctx->out_stride * y);
    u8* yrow = ctx->inp_y + (ctx->inp_y_stride * y);
    u8* urow = ctx->inp_u + (ctx->inp_u_stride * uvy);
    u8* vrow = ctx->inp_v + (ctx->inp_v_stride * uvy);
    for (u32 x = 0; x < ctx->width; ++x) {
      u32 uvx = x / 2;
      u8 y = yrow[x];
      u8 u = urow[uvx];
      u8 v = vrow[uvx];

      i32 c = y;
      i32 d = u - 128;
      i32 e = v - 128;

      i32 rt = c + (i32)(1.402f * e);
      i32 gt = c - (i32)(0.344136f * d + 0.714136f * e);
      i32 bt = c + (i32)(1.772f * d);

      row[x*3+0] = ClampByte(rt);
      row[x*3+1] = ClampByte(gt);
      row[x*3+2] = ClampByte(bt);
    }
  }
}

void Naive_Destroy(Context* ctx)
{
}
