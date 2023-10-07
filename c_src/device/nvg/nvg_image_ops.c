#include "comms.h"
#include "image_ops.h"
#include "nanovg/nanovg.h"

#define REPEAT_XY (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)

int32_t image_ops_create(void* v_ctx, uint32_t width, uint32_t height, void* p_pixels)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  return nvgCreateImageRGBA(p_ctx, width, height, REPEAT_XY, p_pixels);
}

void image_ops_update(void* v_ctx, int32_t image_id, void* p_pixels)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgUpdateImage(p_ctx, image_id, p_pixels);
}

void image_ops_delete(void* v_ctx, int32_t image_id)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgDeleteImage(p_ctx, image_id);
}
