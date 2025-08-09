#include "yuvbench.h"

#include <libswscale/swscale.h>

void yuv_swscale_create(Context* ctx)
{
  av_log_set_level(AV_LOG_ERROR);
  ctx->impl = sws_getContext(ctx->width, ctx->height, AV_PIX_FMT_YUV420P,
                             ctx->width, ctx->height, AV_PIX_FMT_RGB24,
                             SWS_BILINEAR, NULL, NULL, NULL);
}

void yuv_swscale_process(Context* ctx)
{
  const u8* srcs[] = {
    ctx->inp_y,
    ctx->inp_u,
    ctx->inp_v,
  };
  int src_strides[] = {
    ctx->inp_y_stride,
    ctx->inp_u_stride,
    ctx->inp_v_stride,
  };
  u8* dsts[] = {
    ctx->out,
  };
  int dst_strides[] = {
    ctx->out_stride,
  };
  int err = sws_scale(ctx->impl, srcs, src_strides, 0, ctx->height, dsts, dst_strides);
  if (err < 0) {
    char errstr[128];
    av_strerror(err, errstr, sizeof(errstr));
  }
}

void yuv_swscale_destroy(Context* ctx)
{
  sws_freeContext(ctx->impl);
}
