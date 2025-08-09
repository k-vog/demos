#include "yuvbench.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef void(*YUVFunc)(Context*);

static void benchmark(Context* ctx, const char* name, u32 count,
                      YUVFunc yuv_create, YUVFunc yuv_process, YUVFunc yuv_destroy)
{
  printf("%s:\n", name);

  yuv_create(ctx);
  for (u32 i = 0; i < count; ++i) {
    yuv_process(ctx);
  }
  yuv_destroy(ctx);

  // Output files are PPM
  char fname[256];
  snprintf(fname, sizeof(fname), "out-%s.ppm", name);
  FILE * f = fopen(fname, "wb");
  r_assert(f && "couldn't fopen output");
  fprintf(f, "P6\n%d %d\n255\n", ctx->width, ctx->height);
  for (u32 y = 0; y < ctx->height; ++y) {
    const u8* line = ctx->out + (ctx->out_stride * y);
    if (fwrite(line, 1, ctx->width * 3, f) != ctx->width * 3) {
      r_assert(!"fwrite output failed");
    }
  }
  fclose(f);
}

int main(int argc, const char* argv[])
{
  Context ctx = { 0 };

  if (argc != 2) {
    printf("usage: %s <input.kyuv>\n", argv[0]);
    return 1;
  }

  // Input files are PPM except with planar yuv420 data instead of rgb
  // ...and the file magic is "kYUV" instead of "P3"

  FILE* f = fopen(argv[1], "rb");
  r_assert(f && "couldn't fopen input");

  char magic[4];
  if (fread(magic, 1, 4, f) != 4) {
    r_assert(!"couldn't fread magic");
  }
  if (magic[0] != 'k' || magic[1] != 'Y' || magic[2] != 'U' || magic[3] != 'V') {
    r_assert(!"invalid input file, use ./yuv-from-image.py");
  }

  if (fread(&ctx.width, sizeof(ctx.width), 1, f) != 1) {
    r_assert(!"couldn't fread width");
  }
  if (fread(&ctx.height, sizeof(ctx.height), 1, f) != 1) {
    r_assert(!"couldn't fread height");
  }

  r_assert(ctx.width % 2 == 0 && ctx.height % 2 == 0);
  ctx.uv_width = ctx.width / 2;
  ctx.uv_height = ctx.height / 2;

  ctx.alignment = sizeof(void*);

  ctx.out_stride = next_multiple(ctx.width * 3, ctx.alignment);
  ctx.out_len = ctx.out_stride * ctx.height;
  ctx.out = calloc(1, ctx.out_len);

  ctx.inp_y_stride = next_multiple(ctx.width, ctx.alignment);
  ctx.inp_y_len = ctx.inp_y_stride * ctx.height;
  ctx.inp_y = calloc(1, ctx.inp_y_len);

  ctx.inp_u_stride = next_multiple(ctx.uv_width, ctx.alignment);
  ctx.inp_u_len = ctx.inp_u_stride * ctx.uv_height;
  ctx.inp_u = calloc(1, ctx.inp_u_len);

  ctx.inp_v_stride = next_multiple(ctx.uv_width, ctx.alignment);
  ctx.inp_v_len = ctx.inp_v_stride * ctx.uv_height;
  ctx.inp_v = calloc(1, ctx.inp_v_len);

  printf("inp: %s:\n", argv[1]);
  printf("  format: yuv420p\n");
  printf("  size:   %dx%d (Y)\n", ctx.width, ctx.height);
  printf("          %dx%d (U/V)\n", ctx.uv_width, ctx.uv_height);
  printf("  stride: %d (Y)\n", ctx.inp_y_stride);
  printf("          %d (U/V)\n", ctx.inp_u_stride);
  printf("\n");
  printf("out: %s\n", argv[2]);
  printf("  format: rgb24\n");
  printf("  size:   %dx%d\n", ctx.width, ctx.height);
  printf("  stride: %d\n", ctx.out_stride);

  r_assert(ctx.out && ctx.inp_y && ctx.inp_u && ctx.inp_v);

  for (u32 y = 0; y < ctx.height; ++y) {
    if (fread(ctx.inp_y + (ctx.width * y), 1, ctx.width, f) != ctx.width) {
      r_assert(!"eof");
    }
  }
  for (u32 y = 0; y < ctx.uv_height; ++y) {
    if (fread(ctx.inp_u + (ctx.uv_width * y), 1, ctx.uv_width, f) != ctx.uv_width) {
      r_assert(!"eof");
    }
  }
  for (u32 y = 0; y < ctx.uv_height; ++y) {
    if (fread(ctx.inp_v + (ctx.uv_width * y), 1, ctx.uv_width, f) != ctx.uv_width) {
      r_assert(!"eof");
    }
  }

  fclose(f);

  benchmark(&ctx, "naive", 10,
    yuv_naive_create,
    yuv_naive_process,
    yuv_naive_destroy
  );

  benchmark(&ctx, "corevideo", 10,
    yuv_corevideo_create,
    yuv_corevideo_process,
    yuv_corevideo_destroy
  );

  return 0;
}
