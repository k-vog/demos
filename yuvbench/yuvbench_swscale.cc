#include "yuvbench.hh"

extern "C" {
  #include <libswscale/swscale.h>
}

void Swscale_Create(Context* ctx)
{
  av_log_set_level(AV_LOG_ERROR);
  ctx->impl = sws_getContext(ctx->width, ctx->height, AV_PIX_FMT_YUV420P,
                             ctx->width, ctx->height, AV_PIX_FMT_RGB24,
                             SWS_BILINEAR, NULL, NULL, NULL);
}

void Swscale_Process(Context* ctx)
{
  SwsContext* swsctx = (SwsContext*)ctx->impl;
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
  int err = sws_scale(swsctx, srcs, src_strides, 0, ctx->height, dsts, dst_strides);
  if (err < 0) {
    char errstr[128];
    av_strerror(err, errstr, sizeof(errstr));
  }
}

void Swscale_Destroy(Context* ctx)
{
  sws_freeContext((SwsContext*)ctx->impl);
}
