#include "cairo_ctx.h"
#include "device.h"

extern device_info_t g_device_info;

scenic_cairo_ctx_t* scenic_cairo_init(const device_opts_t* p_opts,
                                      device_info_t* p_info)
{
  scenic_cairo_ctx_t* p_ctx = calloc(1, sizeof(scenic_cairo_ctx_t));

  FT_Error status = FT_Init_FreeType(&p_ctx->ft_library);
  if (status != 0) {
    log_error("cairo: FT_Init_FreeType: Error: %d", status);
    free(p_ctx);

    return NULL;
  }

  p_ctx->ratio = 1.0f;
  p_ctx->dist_tolerance = 0.1f * p_ctx->ratio;

  p_info->width = p_opts->width;
  p_info->height = p_opts->height;

  p_ctx->font_size = 10.0; // Cairo default
  p_ctx->text_align = TEXT_ALIGN_LEFT;
  p_ctx->text_base = TEXT_BASE_ALPHABETIC;

  p_ctx->clear_color = (color_rgba_t){
    // black opaque
    .red = 0.0,
    .green = 0.0,
    .blue = 0.0,
    .alpha = 1.0
  };

  p_info->v_ctx = p_ctx;

  p_ctx->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                              p_info->width, p_info->height);

  return p_ctx;
}

void scenic_cairo_fini(scenic_cairo_ctx_t* p_ctx)
{
  cairo_surface_destroy(p_ctx->surface);
  free(p_ctx);
}

void device_begin_cursor_render(driver_data_t* p_data)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;
  cairo_translate(p_ctx->cr, p_data->cursor_pos[0], p_data->cursor_pos[1]);
}

void device_clear_color(float red, float green, float blue, float alpha)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)g_device_info.v_ctx;
  p_ctx->clear_color = (color_rgba_t){
    .red = red,
    .green = green,
    .blue = blue,
    .alpha = alpha
  };
}

char* device_gl_error()
{
  return NULL;
}

void pattern_stack_push(scenic_cairo_ctx_t* p_ctx)
{
  pattern_stack_t* ptr = (pattern_stack_t*)malloc(sizeof(pattern_stack_t));

  ptr->pattern = p_ctx->pattern;

  if (!p_ctx->pattern_stack_head) {
    ptr->next = NULL;
    p_ctx->pattern_stack_head = ptr;
  } else {
    ptr->next = p_ctx->pattern_stack_head;
    p_ctx->pattern_stack_head = ptr;
  }
}

void pattern_stack_pop(scenic_cairo_ctx_t* p_ctx)
{
  pattern_stack_t* ptr = p_ctx->pattern_stack_head;

  if (!ptr) {
    log_error("pattern stack underflow");
  } else {
    p_ctx->pattern = ptr->pattern;
    p_ctx->pattern_stack_head = p_ctx->pattern_stack_head->next;
    free(ptr);
  }
}
