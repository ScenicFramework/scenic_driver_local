#include "cairo_ctx.h"
#include "font_ops.h"

#include <cairo-ft.h>
int32_t font_ops_create(void* v_ctx, font_t* p_font, uint32_t size)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  const char* name = p_font->id.p_data;
  unsigned char* data = p_font->blob.p_data;

  FT_Face ft_face;
  FT_Error ft_status = FT_New_Memory_Face(p_ctx->ft_library, data, size, 0, &ft_face);

  if (ft_status != 0) {
    log_error("cairo: FT_New_Memory_Face: Error: %d", ft_status);
    return -1;
  }

  cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
  cairo_status_t status = cairo_font_face_set_user_data(font_face, NULL,
                                                        ft_face,
                                                        (cairo_destroy_func_t) FT_Done_Face);
  if (status) {
    log_error("cairo: Failed to create font face: %d", status);
    cairo_font_face_destroy(font_face);
    FT_Done_Face(ft_face);
    return -1;
  }

  return (int32_t)font_face;
}
