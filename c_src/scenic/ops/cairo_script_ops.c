#include <cairo.h>
#include <math.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "font.h"
#include "font_ops.h"
#include "image.h"
#include "script.h"
#include "script_ops.h"

extern device_opts_t g_opts;

static const char* log_prefix = "cairo";

void set_fill_pattern(scenic_cairo_ctx_t* p_ctx, cairo_pattern_t* pattern)
{
  p_ctx->pattern.fill = pattern;
}

void set_stroke_pattern(scenic_cairo_ctx_t* p_ctx, cairo_pattern_t* pattern)
{
  p_ctx->pattern.stroke = pattern;
}

void do_fill_stroke(scenic_cairo_ctx_t* p_ctx, bool fill, bool stroke)
{
  if (fill && stroke) {
    cairo_set_source(p_ctx->cr, p_ctx->pattern.fill);
    cairo_fill_preserve(p_ctx->cr);
  }
  else if (fill) {
    cairo_set_source(p_ctx->cr, p_ctx->pattern.fill);
    cairo_fill(p_ctx->cr);
  }

  if (stroke) {
    cairo_set_source(p_ctx->cr, p_ctx->pattern.stroke);
    cairo_stroke(p_ctx->cr);
  }
}

void script_ops_draw_line(void* v_ctx,
                          coordinates_t a,
                          coordinates_t b,
                          bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_line(log_prefix, __func__, log_level_info,
                             a, b, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_move_to(p_ctx->cr, a.x, a.y);
  cairo_line_to(p_ctx->cr, b.x, b.y);

  if (stroke) {
    cairo_set_source(p_ctx->cr, p_ctx->pattern.stroke);
    cairo_stroke(p_ctx->cr);
  }
}

void script_ops_draw_triangle(void* v_ctx,
                              coordinates_t a,
                              coordinates_t b,
                              coordinates_t c,
                              bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_triangle(log_prefix, __func__, log_level_info,
                                 a, b, c, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_move_to(p_ctx->cr, a.x, a.y);
  cairo_line_to(p_ctx->cr, b.x, b.y);
  cairo_line_to(p_ctx->cr, c.x, c.y);
  cairo_close_path(p_ctx->cr);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_quad(void* v_ctx,
                          coordinates_t a,
                          coordinates_t b,
                          coordinates_t c,
                          coordinates_t d,
                          bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_quad(log_prefix, __func__, log_level_info,
                             a, b, c, d, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_move_to(p_ctx->cr, a.x, a.y);
  cairo_line_to(p_ctx->cr, b.x, b.y);
  cairo_line_to(p_ctx->cr, c.x, c.y);
  cairo_line_to(p_ctx->cr, d.x, d.y);
  cairo_close_path(p_ctx->cr);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_rect(void* v_ctx,
                          float w,
                          float h,
                          bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_rect(log_prefix, __func__, log_level_info,
                             w, h, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_rectangle(p_ctx->cr, 0, 0, w, h);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_rrect(void* v_ctx,
                           float w,
                           float h,
                           float radius,
                           bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_rrect(log_prefix, __func__, log_level_info,
                              w, h, radius, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_arc(p_ctx->cr, 0 + radius, 0 + radius, radius, 2 * (M_PI/2), 3 * (M_PI/2));
  cairo_arc(p_ctx->cr, w - radius, 0 + radius, radius, 3 * (M_PI/2), 4 * (M_PI/2));
  cairo_arc(p_ctx->cr, w - radius, h - radius, radius, 0 * (M_PI/2), 1 * (M_PI/2));
  cairo_arc(p_ctx->cr, 0 + radius, h - radius, radius, 1 * (M_PI/2), 2 * (M_PI/2));
  cairo_close_path(p_ctx->cr);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_arc(void* v_ctx,
                         float radius,
                         float radians,
                         bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_arc(log_prefix, __func__, log_level_info,
                            radius, radians, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  if (radians > 0)
    cairo_arc(p_ctx->cr, 0, 0, radius, 0, radians);
  else
    cairo_arc_negative(p_ctx->cr, 0, 0, radius, 0, radians);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_sector(void* v_ctx,
                            float radius,
                            float radians,
                            bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_sector(log_prefix, __func__, log_level_info,
                               radius, radians, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_move_to(p_ctx->cr, 0, 0);
  cairo_line_to(p_ctx->cr, radius, 0);

  if (radians > 0)
    cairo_arc(p_ctx->cr, 0, 0, radius, 0, radians);
  else
    cairo_arc_negative(p_ctx->cr, 0, 0, radius, 0, radians);

  cairo_close_path(p_ctx->cr);

  do_fill_stroke(p_ctx, fill, stroke);
}

void script_ops_draw_circle(void* v_ctx,
                            float radius,
                            bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_circle(log_prefix, __func__, log_level_info,
                               radius, fill, stroke);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_arc(p_ctx->cr, 0, 0, radius, 0, 2 * M_PI);

  do_fill_stroke(p_ctx, fill, stroke);
}

#warning "cairo: script_ops_draw_ellipse unimplemented"

void script_ops_draw_text(void* v_ctx,
                          uint32_t size,
                          const char* text)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_text(log_prefix, __func__, log_level_info,
                             size, text);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_glyph_t* glyphs = NULL;
  int glyph_count;
  cairo_text_cluster_t* clusters = NULL;
  int cluster_count;
  cairo_text_cluster_flags_t cluster_flags;

  cairo_scaled_font_t* scaled_font = cairo_get_scaled_font(p_ctx->cr);
  cairo_status_t status = cairo_scaled_font_text_to_glyphs(scaled_font,
                                                           0, 0,
                                                           text, size,
                                                           &glyphs, &glyph_count,
                                                           &clusters, &cluster_count,
                                                           &cluster_flags);
  cairo_font_extents_t font_extents;
  cairo_scaled_font_extents(scaled_font, &font_extents);

  cairo_text_extents_t text_extents;
  cairo_scaled_font_glyph_extents(scaled_font, glyphs, glyph_count, &text_extents);

  float align_offset;
  switch (p_ctx->text_align) {
  case TEXT_ALIGN_LEFT:
    align_offset = 0;
    break;
  case TEXT_ALIGN_CENTER:
    align_offset = -(text_extents.width / 2);
    break;
  case TEXT_ALIGN_RIGHT:
    align_offset = -(text_extents.width);
    break;
  }

  float base_offset;
  switch (p_ctx->text_base) {
  case TEXT_BASE_TOP:
    base_offset = font_extents.ascent;
    break;
  case TEXT_BASE_MIDDLE:
    base_offset = -((font_extents.descent - font_extents.ascent) / 2);
    break;
  case TEXT_BASE_ALPHABETIC:
    base_offset = 0;
    break;
  case TEXT_BASE_BOTTOM:
    base_offset = -(font_extents.descent);
    break;
  }

  cairo_translate(p_ctx->cr, align_offset, base_offset);
  cairo_set_source(p_ctx->cr, p_ctx->pattern.fill);

  if (status == CAIRO_STATUS_SUCCESS) {
    int glyph_index = 0;
    int byte_index = 0;
    for (int i = 0; i < cluster_count; i++) {
      cairo_text_cluster_t* cluster = &clusters[i];
      cairo_glyph_t* cluster_glyphs = &glyphs[glyph_index];

      cairo_scaled_font_glyph_extents(scaled_font, cluster_glyphs, cluster->num_glyphs, &text_extents);
      cairo_show_glyphs(p_ctx->cr, cluster_glyphs, cluster->num_glyphs);

      glyph_index += cluster->num_glyphs;
      byte_index += cluster->num_bytes;
    }
  } else {
    log_error("%s: cairo_scaled_font_text_to_glyphs: error %d", __func__, status);
  }

  p_ctx->text_align = TEXT_ALIGN_LEFT;
  p_ctx->text_base = TEXT_BASE_ALPHABETIC;
}

#warning "cairo: script_ops_draw_sprites unimplemented"

void script_ops_begin_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_begin_path(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_new_path(p_ctx->cr);
}

void script_ops_close_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_close_path(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_close_path(p_ctx->cr);
}

void script_ops_fill_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_path(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_set_source(p_ctx->cr, p_ctx->pattern.fill);
  cairo_fill(p_ctx->cr);
}

void script_ops_stroke_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_path(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_set_source(p_ctx->cr, p_ctx->pattern.stroke);
  cairo_stroke(p_ctx->cr);
}

void script_ops_move_to(void* v_ctx,
                        coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_move_to(log_prefix, __func__, log_level_info,
                           a);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_move_to(p_ctx->cr, a.x, a.y);
}

void script_ops_line_to(void* v_ctx,
                        coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_line_to(log_prefix, __func__, log_level_info,
                           a);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_line_to(p_ctx->cr, a.x, a.y);
}

#warning "cairo: script_ops_arc_to unimplemented"

void script_ops_bezier_to(void* v_ctx,
                          coordinates_t c0,
                          coordinates_t c1,
                          coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_bezier_to(log_prefix, __func__, log_level_info,
                             c0, c1, a);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_curve_to(p_ctx->cr, c0.x, c0.y, c1.x, c1.y, a.x, a.y);
}

#warning "cairo: script_ops_quadratic_to unimplemented"

void script_ops_push_state(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_push_state(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  pattern_stack_push(p_ctx);
  cairo_save(p_ctx->cr);
}

void script_ops_pop_state(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_pop_state(log_prefix, __func__, log_level_info);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_restore(p_ctx->cr);
  pattern_stack_pop(p_ctx);
}

#warning "cairo: script_ops_scissor unimplemented"

void script_ops_transform(void* v_ctx,
                          float a, float b,
                          float c, float d,
                          float e, float f)
{
  if (g_opts.debug_mode) {
    log_script_ops_transform(log_prefix, __func__, log_level_info,
                             a, b, c, d, e, f);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_matrix_t matrix = {
    a, b, c, d, e, f
  };

  cairo_transform(p_ctx->cr, &matrix);
}

void script_ops_scale(void* v_ctx,
                      float x, float y)
{
  if (g_opts.debug_mode) {
    log_script_ops_scale(log_prefix, __func__, log_level_info,
                         x, y);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_scale(p_ctx->cr, x, y);
}

void script_ops_rotate(void* v_ctx,
                       float radians)
{
  if (g_opts.debug_mode) {
    log_script_ops_rotate(log_prefix, __func__, log_level_info,
                          radians);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_rotate(p_ctx->cr, radians);
}

void script_ops_translate(void* v_ctx,
                          float x, float y)
{
  if (g_opts.debug_mode) {
    log_script_ops_translate(log_prefix, __func__, log_level_info,
                             x, y);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_translate(p_ctx->cr, x, y);
}

void script_ops_fill_color(void* v_ctx,
                           color_rgba_t color)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_color(log_prefix, __func__, log_level_info,
                              color);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_pattern_t* color_rgba = cairo_pattern_create_rgba(color.red / 255.0f,
                                                          color.green / 255.0f,
                                                          color.blue / 255.0f,
                                                          color.alpha / 255.0f);
  set_fill_pattern(p_ctx, color_rgba);
}

void script_ops_fill_linear(void* v_ctx,
                            coordinates_t start, coordinates_t end,
                            color_rgba_t color_start, color_rgba_t color_end)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_linear(log_prefix, __func__, log_level_info,
                               start, end, color_start, color_end);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_pattern_t* linear_gradient = cairo_pattern_create_linear(start.x, start.y,
                                                                 end.x, end.y);
  cairo_pattern_add_color_stop_rgba(linear_gradient, 0.0,
                                    color_start.red / 255.0f,
                                    color_start.green / 255.0f,
                                    color_start.blue / 255.0f,
                                    color_start.alpha / 255.0f);
  cairo_pattern_add_color_stop_rgba(linear_gradient, 1.0,
                                    color_end.red / 255.0f,
                                    color_end.green / 255.0f,
                                    color_end.blue / 255.0f,
                                    color_end.alpha / 255.0f);
  set_fill_pattern(p_ctx, linear_gradient);
}

void script_ops_fill_radial(void* v_ctx,
                            coordinates_t center,
                            float inner_radius,
                            float outer_radius,
                            color_rgba_t color_start,
                            color_rgba_t color_end)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_radial(log_prefix, __func__, log_level_info,
                               center, inner_radius, outer_radius, color_start, color_end);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_pattern_t* radial_gradient = cairo_pattern_create_radial(center.x, center.y,
                                                                 inner_radius,
                                                                 center.x, center.y,
                                                                 outer_radius);
  cairo_pattern_add_color_stop_rgba(radial_gradient, 0.0,
                                    color_start.red / 255.0f,
                                    color_start.green / 255.0f,
                                    color_start.blue / 255.0f,
                                    color_start.alpha / 255.0f);
  cairo_pattern_add_color_stop_rgba(radial_gradient, 1.0,
                                    color_end.red / 255.0f,
                                    color_end.green / 255.0f,
                                    color_end.blue / 255.0f,
                                    color_end.alpha / 255.0f);
  set_fill_pattern(p_ctx, radial_gradient);
}

void script_ops_fill_image(void* v_ctx, sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_image(log_prefix, __func__, log_level_info,
                              id);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  // get the mapped image_id for this image_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  image_data_t* image_data = (image_data_t*)p_image->image_id;

  cairo_set_antialias(p_ctx->cr, CAIRO_ANTIALIAS_NONE);
  set_fill_pattern(p_ctx, image_data->pattern);
}

#warning "cairo: script_ops_fill_stream unimplemented"

void script_ops_stroke_width(void* v_ctx,
                             float w)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_width(log_prefix, __func__, log_level_info,
                                w);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_set_line_width(p_ctx->cr, w);
}

void script_ops_stroke_color(void* v_ctx,
                             color_rgba_t color)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_color(log_prefix, __func__, log_level_info,
                                color);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_pattern_t* color_rgba = cairo_pattern_create_rgba(color.red / 255.0f,
                                                          color.green / 255.0f,
                                                          color.blue / 255.0f,
                                                          color.alpha / 255.0f);
  set_stroke_pattern(p_ctx, color_rgba);
}

#warning "cairo: script_ops_stroke_linear unimplemented"

#warning "cairo: script_ops_stroke_radial unimplemented"

#warning "cairo: script_ops_stroke_image unimplemented"

#warning "cairo: script_ops_stroke_stream unimplemented"

void script_ops_line_cap(void* v_ctx,
                         line_cap_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_line_cap(log_prefix, __func__, log_level_info,
                            type);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  switch(type) {
  case LINE_CAP_BUTT:
    cairo_set_line_cap(p_ctx->cr, CAIRO_LINE_CAP_BUTT);
    break;
  case LINE_CAP_ROUND:
    cairo_set_line_cap(p_ctx->cr, CAIRO_LINE_CAP_ROUND);
    break;
  case LINE_CAP_SQUARE:
    cairo_set_line_cap(p_ctx->cr, CAIRO_LINE_CAP_SQUARE);
    break;
  }
}

void script_ops_line_join(void* v_ctx,
                          line_join_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_line_join(log_prefix, __func__, log_level_info,
                             type);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  switch(type) {
  case LINE_JOIN_BEVEL:
    cairo_set_line_join(p_ctx->cr, CAIRO_LINE_JOIN_BEVEL);
    break;
  case LINE_JOIN_ROUND:
    cairo_set_line_join(p_ctx->cr, CAIRO_LINE_JOIN_ROUND);
    break;
  case LINE_JOIN_MITER:
    cairo_set_line_join(p_ctx->cr, CAIRO_LINE_JOIN_MITER);
    break;
  }
}

void script_ops_miter_limit(void* v_ctx,
                            uint32_t limit)
{
  if (g_opts.debug_mode) {
    log_script_ops_miter_limit(log_prefix, __func__, log_level_info,
                               limit);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  cairo_set_miter_limit(p_ctx->cr, limit);
}

void script_ops_font(void* v_ctx,
                     sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_font(log_prefix, __func__, log_level_info,
                        id);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  font_t* p_font = get_font(id);

  cairo_set_font_face(p_ctx->cr, (cairo_font_face_t*)p_font->font_id);
}

void script_ops_font_size(void* v_ctx,
                          float size)
{
  if (g_opts.debug_mode) {
    log_script_ops_font_size(log_prefix, __func__, log_level_info,
                             size);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  p_ctx->font_size = size;
  cairo_set_font_size(p_ctx->cr, size);
}

void script_ops_text_align(void* v_ctx,
                           text_align_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_text_align(log_prefix, __func__, log_level_info,
                              type);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  p_ctx->text_align = type;
}

void script_ops_text_base(void* v_ctx,
                          text_base_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_text_base(log_prefix, __func__, log_level_info,
                             type);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  p_ctx->text_base = type;
}
