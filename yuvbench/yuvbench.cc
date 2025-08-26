#include "yuvbench.hh"

#include "com_timer.h"

typedef void(*YUVFunc)(Context*);

static void Benchmark(Context* ctx, const char* name, u32 count,
                      YUVFunc YUV_Create, YUVFunc YUV_Process, YUVFunc YUV_Destroy)
{
  u64 t0 = 0;
  u64 t1 = 0;
  // 1: benchmark create(), process(), destroy()
  for (u32 i = 0; i < 5; ++i) {
    YUV_Create(ctx);
    YUV_Process(ctx);
    YUV_Destroy(ctx);
  }
  t0 = timer_timestamp();
  for (u32 i = 0; i < count; ++i) {
    YUV_Create(ctx);
    YUV_Process(ctx);
    YUV_Destroy(ctx);
  }
  t1 = timer_timestamp();
  printf("%-12s %15.2fus\n", name, timer_elapsed(t0, t1) / 1000.0f / (f64)count);

  // 2: benchmark process()
  YUV_Create(ctx);
  for (u32 i = 0; i < 5; ++i) {
    YUV_Process(ctx);
  }
  t0 = timer_timestamp();
  for (u32 i = 0; i < count; ++i) {
    YUV_Process(ctx);
  }
  t1 = timer_timestamp();
  YUV_Destroy(ctx);
  printf("%-10s r %15.2fus\n", name, timer_elapsed(t0, t1) / 1000.0f / (f64)count);

  // Output files are PPM
  char fname[256];
  snprintf(fname, sizeof(fname), "out-%s.ppm", name);
  FILE * f = fopen(fname, "wb");
  ASSERT(f && "couldn't fopen output");
  fprintf(f, "P6\n%d %d\n255\n", ctx->width, ctx->height);
  for (u32 y = 0; y < ctx->height; ++y) {
    const u8* line = ctx->out + (ctx->out_stride * y);
    if (fwrite(line, 1, ctx->width * 3, f) != ctx->width * 3) {
      ASSERT(!"fwrite output failed");
    }
  }
  fclose(f);
}

int main(int argc, const char* argv[])
{
  Context ctx = { };

  if (argc != 2) {
    printf("usage: %s <input.kyuv>\n", argv[0]);
    return 1;
  }

  // Input files are PPM except with planar yuv420 data instead of rgb
  // ...and the file magic is "kYUV" instead of "P3"

  FILE* f = fopen(argv[1], "rb");
  ASSERT(f && "couldn't fopen input");

  char magic[4];
  if (fread(magic, 1, 4, f) != 4) {
    ASSERT(!"couldn't fread magic");
  }
  if (magic[0] != 'k' || magic[1] != 'Y' || magic[2] != 'U' || magic[3] != 'V') {
    ASSERT(!"invalid input file, use ./yuv-from-image.py");
  }

  if (fread(&ctx.width, sizeof(ctx.width), 1, f) != 1) {
    ASSERT(!"couldn't fread width");
  }
  if (fread(&ctx.height, sizeof(ctx.height), 1, f) != 1) {
    ASSERT(!"couldn't fread height");
  }

  ASSERT(ctx.width % 2 == 0 && ctx.height % 2 == 0);
  ctx.uv_width = ctx.width / 2;
  ctx.uv_height = ctx.height / 2;

  ctx.alignment = sizeof(void*);

  ctx.out_stride = Align<usize>(ctx.width * 3, ctx.alignment);
  ctx.out_len = ctx.out_stride * ctx.height;
  ctx.out = MemAllocZ<u8>(ctx.out_len);

  ctx.inp_y_stride = Align<usize>(ctx.width, ctx.alignment);
  ctx.inp_y_len = ctx.inp_y_stride * ctx.height;
  ctx.inp_y = MemAllocZ<u8>(ctx.inp_y_len);

  ctx.inp_u_stride = Align<usize>(ctx.uv_width, ctx.alignment);
  ctx.inp_u_len = ctx.inp_u_stride * ctx.uv_height;
  ctx.inp_u = MemAllocZ<u8>(ctx.inp_u_len);

  ctx.inp_v_stride = Align<usize>(ctx.uv_width, ctx.alignment);
  ctx.inp_v_len = ctx.inp_v_stride * ctx.uv_height;
  ctx.inp_v = MemAllocZ<u8>(ctx.inp_v_len);

  printf("inp: %s:\n", argv[1]);
  printf("  format: yuv420p\n");
  printf("  size:   %dx%d (Y)\n", ctx.width, ctx.height);
  printf("          %dx%d (U/V)\n", ctx.uv_width, ctx.uv_height);
  printf("  stride: %d (Y)\n", ctx.inp_y_stride);
  printf("          %d (U/V)\n", ctx.inp_u_stride);
  printf("\n");
  printf("out: out-*.ppm\n");
  printf("  format: rgb24\n");
  printf("  size:   %dx%d\n", ctx.width, ctx.height);
  printf("  stride: %d\n", ctx.out_stride);
  printf("\n");

  for (u32 y = 0; y < ctx.height; ++y) {
    if (fread(ctx.inp_y + (ctx.width * y), 1, ctx.width, f) != ctx.width) {
      ASSERT(!"eof");
    }
  }
  for (u32 y = 0; y < ctx.uv_height; ++y) {
    if (fread(ctx.inp_u + (ctx.uv_width * y), 1, ctx.uv_width, f) != ctx.uv_width) {
      ASSERT(!"eof");
    }
  }
  for (u32 y = 0; y < ctx.uv_height; ++y) {
    if (fread(ctx.inp_v + (ctx.uv_width * y), 1, ctx.uv_width, f) != ctx.uv_width) {
      ASSERT(!"eof");
    }
  }

  fclose(f);

  u32 count = 100;

#if __APPLE__
  Benchmark(&ctx, "corevideo", count,
    CoreVideo_Create,
    CoreVideo_Process,
    CoreVideo_Destroy
  );
#endif

  Benchmark(&ctx, "naive", count,
    Naive_Create,
    Naive_Process,
    Naive_Destroy
  );

  Benchmark(&ctx, "swscale", count,
    Swscale_Create,
    Swscale_Process,
    Swscale_Destroy
  );

  Benchmark(&ctx, "vulkan", count,
    Vulkan_Create,
    Vulkan_Process,
    Vulkan_Destroy
  );

  return 0;
}
