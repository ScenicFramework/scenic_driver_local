#include "comms.h"
#include "image.h"
#include "image_ops.h"
#include "nanovg.h"

#define REPEAT_XY (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)

int32_t image_ops_create(void* v_ctx, uint32_t width, uint32_t height, void* p_pixels)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  return nvgCreateImageRGBA(p_ctx, width, height, REPEAT_XY, p_pixels);
}

void image_ops_update(void* v_ctx, uint32_t image_id, void* p_pixels)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgUpdateImage(p_ctx, image_id, p_pixels);
}

void image_ops_delete(void* v_ctx, uint32_t image_id)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgDeleteImage(p_ctx, image_id);
}

//=============================================================================
// called when rendering scripts

//---------------------------------------------------------
void set_fill_image(void* v_ctx, sid_t id)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->image_id, &w, &h);


  // the image is loaded and ready for use
  nvgFillPaint(p_ctx,
               nvgImagePattern(p_ctx, 0, 0, w, h, 0, p_image->image_id, 1.0));
}

//---------------------------------------------------------
void set_stroke_image(void* v_ctx, sid_t id)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->image_id, &w, &h);

  // the image is loaded and ready for use
  nvgStrokePaint(p_ctx,
                 nvgImagePattern(p_ctx, 0, 0, w, h, 0, p_image->image_id, 1.0));
}

//---------------------------------------------------------
// see: https://github.com/memononen/nanovg/issues/348
void draw_image(void* v_ctx, sid_t id,
                float sx, float sy, float sw, float sh,
                float dx, float dy, float dw, float dh)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  float ax, ay;
  NVGpaint img_pattern;

  // get the mapped nvg_id for this driver_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int iw,ih;
  nvgImageSize(p_ctx, p_image->image_id, &iw, &ih);

  // Aspect ration of pixel in x an y dimensions. This allows us to scale
  // the sprite to fill the whole rectangle.
  ax = dw / sw;
  ay = dh / sh;

  // create the temporary pattern
  img_pattern = nvgImagePattern(p_ctx,
                                dx - sx*ax, dy - sy*ay, (float)iw*ax, (float)ih*ay,
                                0, p_image->image_id, 1.0);

  // draw the image into a rect
  nvgBeginPath(p_ctx);
  nvgRect(p_ctx, dx, dy, dw, dh);
  nvgFillPaint(p_ctx, img_pattern);
  nvgFill(p_ctx);

  // the data for the paint pattern is a struct on the stack.
  // no need to clean it up
}
