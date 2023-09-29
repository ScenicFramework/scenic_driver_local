#include "cairo_ctx.h"
#include "font_ops.h"

#include <cairo-ft.h>

static int maxi(int a, int b) { return a > b ? a : b; }

static
font_data_t* alloc_font_data(scenic_cairo_ctx_t* p_ctx, cairo_font_face_t* font_face)
{
  font_data_t* font = NULL;
  int i;

  for (i = 0; i < p_ctx->fonts_used; i++) {
    if (p_ctx->fonts[i].id == 0) {
      font = &p_ctx->fonts[i];
      break;
    }
  }

  if (!font) {
    if (p_ctx->fonts_used + 1 > p_ctx->fonts_count) {
      font_data_t* fonts;
      int fonts_count = maxi(p_ctx->fonts_count + 1, 4) + p_ctx->fonts_used / 2; // 1.5x over allocation
      fonts = (font_data_t*)realloc(p_ctx->fonts, sizeof(font_data_t) * fonts_count);
      if (!fonts) return NULL;
      p_ctx->fonts = fonts;
      p_ctx->fonts_count = fonts_count;
    }
    font = &p_ctx->fonts[p_ctx->fonts_used++];
  }

  memset(font, 0, sizeof(*font));
  font->id = ++p_ctx->highest_font_id;
  font->font_face = font_face;

  return font;
}

font_data_t* find_font(scenic_cairo_ctx_t* p_ctx, int id)
{
  int i;
  for (i = 0; i < p_ctx->fonts_used; i++) {
    if (p_ctx->fonts[i].id == id) {
      return &p_ctx->fonts[i];
    }
  }
  return NULL;
}

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

  font_data_t* font_data = alloc_font_data(p_ctx, font_face);
  if (!font_data) return -1;

  return font_data->id;
}
