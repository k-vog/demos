#include "yuvbench.h"

#include <Accelerate/Accelerate.h>
#include <CoreVideo/CoreVideo.h>

static const char* cv_error_string(vImage_Error err)
{
  switch (err) {
#define BIND_ERROR(errcode) case errcode: return #errcode
    BIND_ERROR(kvImageNoError);
    BIND_ERROR(kvImageRoiLargerThanInputBuffer);
    BIND_ERROR(kvImageInvalidKernelSize);
    BIND_ERROR(kvImageInvalidEdgeStyle);
    BIND_ERROR(kvImageInvalidOffset_X);
    BIND_ERROR(kvImageInvalidOffset_Y);
    BIND_ERROR(kvImageMemoryAllocationError);
    BIND_ERROR(kvImageNullPointerArgument);
    BIND_ERROR(kvImageInvalidParameter);
    BIND_ERROR(kvImageBufferSizeMismatch);
    BIND_ERROR(kvImageUnknownFlagsBit);
    BIND_ERROR(kvImageInternalError);
    BIND_ERROR(kvImageInvalidRowBytes);
    BIND_ERROR(kvImageInvalidImageFormat);
    BIND_ERROR(kvImageColorSyncIsAbsent);
    BIND_ERROR(kvImageOutOfPlaceOperationRequired);
    BIND_ERROR(kvImageInvalidImageObject);
    BIND_ERROR(kvImageInvalidCVImageFormat);
    BIND_ERROR(kvImageUnsupportedConversion);
    BIND_ERROR(kvImageCoreVideoIsAbsent);
#undef BIND_ERROR
    default: return "invalid error enum";
  }
}

typedef struct CoreVideoContext CoreVideoContext;
struct CoreVideoContext
{
  // ARGB intermediate buffer
  u8*    argb;
  size_t argb_len;
  size_t argb_stride;
  // Accelerate API buffer views
  vImage_Buffer ybuf;
  vImage_Buffer ubuf;
  vImage_Buffer vbuf;
  vImage_Buffer argbbuf;
  vImage_Buffer rgbbuf;
  // YUV conversion context
  vImage_YpCbCrToARGB conv;
};

void yuv_corevideo_create(Context* ctx)
{
  CoreVideoContext* cv = calloc(1, sizeof(CoreVideoContext));
  r_assert(cv);
  cv->argb_stride = next_multiple(ctx->width * 4, ctx->alignment);
  cv->argb_len = cv->argb_stride * ctx->height;
  cv->argb = calloc(1, cv->argb_len);
  r_assert(cv->argb);

  cv->ybuf.data = ctx->inp_y;
  cv->ybuf.width = ctx->width;
  cv->ybuf.height = ctx->height;
  cv->ybuf.rowBytes = ctx->inp_y_stride;

  cv->ubuf.data = ctx->inp_u;
  cv->ubuf.width = ctx->uv_width;
  cv->ubuf.height = ctx->uv_width;
  cv->ubuf.rowBytes = ctx->inp_u_stride;

  cv->vbuf.data = ctx->inp_v;
  cv->vbuf.width = ctx->uv_width;
  cv->vbuf.height = ctx->uv_width;
  cv->vbuf.rowBytes = ctx->inp_v_stride;

  cv->argbbuf.data = cv->argb;
  cv->argbbuf.width = ctx->width;
  cv->argbbuf.height = ctx->height;
  cv->argbbuf.rowBytes = cv->argb_stride;

  cv->rgbbuf.data = ctx->out;
  cv->rgbbuf.width = ctx->width;
  cv->rgbbuf.height = ctx->height;
  cv->rgbbuf.rowBytes = ctx->out_stride;

  vImage_YpCbCrPixelRange pixel_range = {
    .Yp_bias = 0,
    .CbCr_bias = 128,
    .YpRangeMax = 255,
    .CbCrRangeMax = 255,
    .YpMax = 255,
    .YpMin = 1,
    .CbCrMax = 255,
    .CbCrMin = 0
  };
  vImageConvert_YpCbCrToARGB_GenerateConversion(kvImage_YpCbCrToARGBMatrix_ITU_R_601_4,
                                                &pixel_range,
                                                &cv->conv,
                                                kvImage420Yp8_Cb8_Cr8,
                                                kvImageARGB8888,
                                                kvImageNoFlags);

  ctx->impl = cv;
}

void yuv_corevideo_process(Context* ctx)
{
  CoreVideoContext* cv = ctx->impl;

  // Apple doesn't support acclerated yuv420p -> rgb, only to argb
  // so do that, then convert argb -> rgb

  vImage_Error err = kvImageNoError;
  err = vImageConvert_420Yp8_Cb8_Cr8ToARGB8888(&cv->ybuf, &cv->ubuf, &cv->vbuf,
                                               &cv->argbbuf,
                                               &cv->conv, NULL, 0xFF, kvImageNoFlags);
  if (err != kvImageNoError) {
    printf("yuv420p -> argb failed: %s", cv_error_string(err));
    r_assert(0);
  }
  err = vImageConvert_ARGB8888toRGB888(&cv->argbbuf, &cv->rgbbuf, kvImageNoFlags);
  if (err != kvImageNoError) {
    printf("argb -> argb failed: %s", cv_error_string(err));
    r_assert(0);
  }
}

void yuv_corevideo_destroy(Context* ctx)
{
  free(ctx->impl);
}
