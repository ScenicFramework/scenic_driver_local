#include "font_ops.h"
#include "nanovg.h"

int32_t font_ops_create(void* v_ctx, font_t* p_font, uint32_t size)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  return nvgCreateFontMem(p_ctx,
                          p_font->id.p_data, p_font->blob.p_data, size,
                          false // tells nvg to NOT free p_font->blob.p_data when releasing font
                          );
}
