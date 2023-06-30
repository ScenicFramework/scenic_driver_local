#include <string.h>

#include "comms.h"
#include "font_ops.h"
#include "nanovg.h"

int32_t font_ops_create(void* v_ctx, font_t* p_font, uint32_t size)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  return nvgCreateFontMem(p_ctx,
                          p_font->id.p_data, p_font->blob.p_data, size,
                          false); // tells nvg to NOT free p_font->blob.p_data when releasing font
}

//---------------------------------------------------------
void set_font(sid_t id, void* v_ctx)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  // unfortunately, nvgFindFont expects a zero terminated C string
  char* p_font = calloc(1, id.size + 1);
  if ( !p_font ) {
    log_error("Unable to alloc temp font id buffer");
    return;
  }
  memcpy(p_font, id.p_data, id.size);

  int font_id = nvgFindFont(p_ctx, p_font);
  if (font_id >= 0) {
    nvgFontFaceId(p_ctx, font_id);
  }

  free( p_font );
}
