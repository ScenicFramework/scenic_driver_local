#include <stddef.h>

#include "comms.h"
#include "font.h"
#include "image.h"
#include "script_ops.h"
#include "scenic_types.h"
#include "nanovg.h"

extern device_opts_t g_opts;

static const char* log_prefix = "nvg";

void script_ops_draw_line(void* v_ctx,
                          coordinates_t a,
                          coordinates_t b,
                          bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_line(log_prefix, __func__, log_level_info,
                             a, b, stroke);
  }
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, a.x, a.y);
  nvgLineTo(p_ctx, b.x, b.y);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, a.x, a.y);
  nvgLineTo(p_ctx, b.x, b.y);
  nvgLineTo(p_ctx, c.x, c.y);
  nvgClosePath(p_ctx);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, a.x, a.y);
  nvgLineTo(p_ctx, b.x, b.y);
  nvgLineTo(p_ctx, c.x, c.y);
  nvgLineTo(p_ctx, d.x, d.y);
  nvgClosePath(p_ctx);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgRect(p_ctx, 0, 0, w, h);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, 0, 0, w, h, radius);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgArc(p_ctx,
         0, 0,
         radius, 0, radians,
         (radians > 0)
          ? NVG_CW
          : NVG_CCW);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, 0, 0);
  nvgLineTo(p_ctx, radius, 0);
  nvgArc(p_ctx,
          0, 0,
          radius, 0, radians,
          (radians > 0)
           ? NVG_CW
           : NVG_CCW);
  nvgClosePath(p_ctx);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
}

void script_ops_draw_circle(void* v_ctx,
                            float radius,
                            bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_circle(log_prefix, __func__, log_level_info,
                               radius, fill, stroke);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgCircle(p_ctx, 0, 0, radius);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
}

void script_ops_draw_ellipse(void* v_ctx,
                             float radius0,
                             float radius1,
                             bool fill, bool stroke)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_ellipse(log_prefix, __func__, log_level_info,
                                radius0, radius1, fill, stroke);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
  nvgEllipse(p_ctx, 0, 0, radius0, radius1);
  if (fill) nvgFill(p_ctx);
  if (stroke) nvgStroke(p_ctx);
}

void script_ops_draw_text(void* v_ctx,
                          uint32_t size,
                          const char* text)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_text(log_prefix, __func__, log_level_info,
                             size, text);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  float x = 0;
  float y = 0;
  const char* start = text;
  const char* end = start + size;
  float lineh;
  nvgTextMetrics(p_ctx, NULL, NULL, &lineh);
  NVGtextRow rows[3];
  int nrows, i;

  // up to this code to break the lines...
  while ((nrows = nvgTextBreakLines(p_ctx, start, end, 1000, rows, 3)))
  {
    for (i = 0; i < nrows; i++)
    {
      NVGtextRow* row = &rows[i];
      nvgText(p_ctx, x, y, row->start, row->end);
      y += lineh;
    }
    // Keep going...
    start = rows[nrows - 1].next;
  }
}

//---------------------------------------------------------
// see: https://github.com/memononen/nanovg/issues/348
static void draw_image(void* v_ctx,
                       sid_t id,
                       const sprite_t s)
{
  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  float ax, ay;
  NVGpaint img_pattern;
  
  // get the mapped image_id for this driver_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int iw,ih;
  nvgImageSize(p_ctx, p_image->image_id, &iw, &ih);

  // Aspect ration of pixel in x an y dimensions. This allows us to scale
  // the sprite to fill the whole rectangle.
  ax = s.dw / s.sw;
  ay = s.dh / s.sh;

  // create the temporary pattern
  img_pattern = nvgImagePattern(p_ctx,
                                s.dx - s.sx*ax, s.dy - s.sy*ay,
                                (float)iw*ax, (float)ih*ay,
                                0, p_image->image_id, 1.0);

  // draw the image into a rect
  nvgBeginPath(p_ctx);
  nvgRect(p_ctx, s.dx, s.dy, s.dw, s.dh);
  nvgFillPaint(p_ctx, img_pattern);
  nvgFill(p_ctx);

  // the data for the paint pattern is a struct on the stack.
  // no need to clean it up
}

void script_ops_draw_sprites(void* v_ctx,
                             sid_t id,
                             uint32_t count,
                             const sprite_t* sprites)
{
  if (g_opts.debug_mode) {
    log_script_ops_draw_sprites(log_prefix, __func__, log_level_info,
                                id, count, sprites);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  for (int i = 0; i < count; i++) {
    draw_image(p_ctx, id, sprites[i]);
  }
}

void script_ops_begin_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_begin_path(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBeginPath(p_ctx);
}

void script_ops_close_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_close_path(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgClosePath(p_ctx);
}

void script_ops_fill_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_path(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgFill(p_ctx);
}

void script_ops_stroke_path(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_path(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgStroke(p_ctx);
}

void script_ops_move_to(void* v_ctx,
                        coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_move_to(log_prefix, __func__, log_level_info,
                           a);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgMoveTo(p_ctx, a.x, a.y);
}

void script_ops_line_to(void* v_ctx,
                        coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_line_to(log_prefix, __func__, log_level_info,
                           a);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgLineTo(p_ctx, a.x, a.y);
}

void script_ops_arc_to(void* v_ctx,
                       coordinates_t a,
                       coordinates_t b,
                       float radius)
{
  if (g_opts.debug_mode) {
    log_script_ops_arc_to(log_prefix, __func__, log_level_info,
                          a, b, radius);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgArcTo(p_ctx, a.x, a.y, b.x, b.y, radius);
}

void script_ops_bezier_to(void* v_ctx,
                          coordinates_t c0,
                          coordinates_t c1,
                          coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_bezier_to(log_prefix, __func__, log_level_info,
                             c0, c1, a);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgBezierTo(p_ctx, c0.x, c0.y, c1.x, c1.y, a.x, a.y);
}

void script_ops_quadratic_to(void* v_ctx,
                             coordinates_t c,
                             coordinates_t a)
{
  if (g_opts.debug_mode) {
    log_script_ops_quadratic_to(log_prefix, __func__, log_level_info,
                                c, a);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgQuadTo(p_ctx, c.x, c.y, a.x, a.y);
}

void script_ops_push_state(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_push_state(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgSave(p_ctx);
}

void script_ops_pop_state(void* v_ctx)
{
  if (g_opts.debug_mode) {
    log_script_ops_pop_state(log_prefix, __func__, log_level_info);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgRestore(p_ctx);
}

void script_ops_scissor(void* v_ctx,
                        float w, float h)
{
  if (g_opts.debug_mode) {
    log_script_ops_scissor(log_prefix, __func__, log_level_info,
                           w, h);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgScissor(p_ctx, 0, 0, w, h);
}

void script_ops_transform(void* v_ctx,
                          float a, float b,
                          float c, float d,
                          float e, float f)
{
  if (g_opts.debug_mode) {
    log_script_ops_transform(log_prefix, __func__, log_level_info,
                             a, b, c, d, e, f);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgTransform(p_ctx, a, b, c, d, e, f);
}

void script_ops_scale(void* v_ctx,
                      float x, float y)
{
  if (g_opts.debug_mode) {
    log_script_ops_scale(log_prefix, __func__, log_level_info,
                         x, y);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgScale(p_ctx, x, y);
}

void script_ops_rotate(void* v_ctx,
                       float radians)
{
  if (g_opts.debug_mode) {
    log_script_ops_rotate(log_prefix, __func__, log_level_info,
                          radians);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgRotate(p_ctx, radians);
}

void script_ops_translate(void* v_ctx,
                          float x, float y)
{
  if (g_opts.debug_mode) {
    log_script_ops_translate(log_prefix, __func__, log_level_info,
                             x, y);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgTranslate(p_ctx, x, y);
}

void script_ops_fill_color(void* v_ctx,
                           color_rgba_t color)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_color(log_prefix, __func__, log_level_info,
                              color);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgFillColor(p_ctx, nvgRGBA(color.red, color.green, color.blue, color.alpha));
}

void script_ops_fill_linear(void* v_ctx,
                            coordinates_t start,
                            coordinates_t end,
                            color_rgba_t color_start,
                            color_rgba_t color_end)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_linear(log_prefix, __func__, log_level_info,
                               start, end, color_start, color_end);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgFillPaint(p_ctx,
               nvgLinearGradient(p_ctx,
                                 start.x, start.y,
                                 end.x, end.y,
                                 nvgRGBA(color_start.red,
                                         color_start.green,
                                         color_start.blue,
                                         color_start.alpha),
                                 nvgRGBA(color_end.red,
                                         color_end.green,
                                         color_end.blue,
                                         color_end.alpha)));
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgFillPaint(p_ctx,
               nvgRadialGradient(p_ctx,
                                 center.x, center.y,
                                 inner_radius, outer_radius,
                                 nvgRGBA(color_start.red,
                                         color_start.green,
                                         color_start.blue,
                                         color_start.alpha),
                                 nvgRGBA(color_end.red,
                                         color_end.green,
                                         color_end.blue,
                                         color_end.alpha)));
}

void script_ops_fill_image(void* v_ctx,
                           sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_image(log_prefix, __func__, log_level_info,
                              id);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  // get the mapped image_id for this image_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->image_id, &w, &h);

  // the image is loaded and ready for use
  nvgFillPaint(p_ctx,
               nvgImagePattern(p_ctx,
                               0, 0, w, h, 0, p_image->image_id, 1.0));
}

void script_ops_fill_stream(void* v_ctx,
                            sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_fill_stream(log_prefix, __func__, log_level_info,
                               id);
  }

  script_ops_fill_image(v_ctx, id);
}

void script_ops_stroke_width(void* v_ctx,
                             float w)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_width(log_prefix, __func__, log_level_info,
                                w);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgStrokeWidth(p_ctx, w);
}

void script_ops_stroke_color(void* v_ctx,
                             color_rgba_t color)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_color(log_prefix, __func__, log_level_info,
                                color);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgStrokeColor(p_ctx, nvgRGBA(color.red,
                                color.green,
                                color.blue,
                                color.alpha));
}

void script_ops_stroke_linear(void* v_ctx,
                              coordinates_t start,
                              coordinates_t end,
                              color_rgba_t color_start,
                              color_rgba_t color_end)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_linear(log_prefix, __func__, log_level_info,
                                 start, end, color_start, color_end);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgStrokePaint(p_ctx,
                 nvgLinearGradient(p_ctx,
                                   start.x, start.y,
                                   end.x, end.y,
                                   nvgRGBA(color_start.red,
                                           color_start.green,
                                           color_start.blue,
                                           color_start.alpha),
                                   nvgRGBA(color_end.red,
                                           color_end.green,
                                           color_end.blue,
                                           color_end.alpha)));
}

void script_ops_stroke_radial(void* v_ctx,
                              coordinates_t center,
                              float inner_radius,
                              float outer_radius,
                              color_rgba_t color_start,
                              color_rgba_t color_end)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_radial(log_prefix, __func__, log_level_info,
                                 center, inner_radius, outer_radius, color_start, color_end);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgStrokePaint(p_ctx,
                 nvgRadialGradient(p_ctx,
                                   center.x, center.y,
                                   inner_radius, outer_radius,
                                   nvgRGBA(color_start.red,
                                           color_start.green,
                                           color_start.blue,
                                           color_start.alpha),
                                   nvgRGBA(color_end.red,
                                           color_end.green,
                                           color_end.blue,
                                           color_end.alpha)));
}

void script_ops_stroke_image(void* v_ctx,
                             sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_image(log_prefix, __func__, log_level_info,
                                id);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  // get the mapped image_id for this image_id
  image_t* p_image = get_image(id);
  if (!p_image) return;

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->image_id, &w, &h);

  // the image is loaded and ready for use
  nvgStrokePaint(p_ctx,
                 nvgImagePattern(p_ctx,
                                 0, 0, w, h, 0, p_image->image_id, 1.0));
}

void script_ops_stroke_stream(void* v_ctx,
                              sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_stroke_stream(log_prefix, __func__, log_level_info,
                                 id);
  }

  script_ops_stroke_image(v_ctx, id);
}

void script_ops_line_cap(void* v_ctx,
                         line_cap_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_line_cap(log_prefix, __func__, log_level_info,
                            type);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  switch(type) {
  case LINE_CAP_BUTT:
    nvgLineCap(p_ctx, NVG_BUTT);
    break;
  case LINE_CAP_ROUND:
    nvgLineCap(p_ctx, NVG_ROUND);
    break;
  case LINE_CAP_SQUARE:
    nvgLineCap(p_ctx, NVG_SQUARE);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  switch(type) {
  case LINE_JOIN_BEVEL:
    nvgLineJoin(p_ctx, NVG_BEVEL);
    break;
  case LINE_CAP_ROUND:
    nvgLineJoin(p_ctx, NVG_ROUND);
    break;
  case LINE_JOIN_MITER:
    nvgLineJoin(p_ctx, NVG_MITER);
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

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgMiterLimit(p_ctx, limit);
}

void script_ops_font(void* v_ctx,
                     sid_t id)
{
  if (g_opts.debug_mode) {
    log_script_ops_font(log_prefix, __func__, log_level_info,
                        id);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  font_t* p_font = get_font(id);
  if (p_font)
    nvgFontFaceId(p_ctx, p_font->font_id);
}

void script_ops_font_size(void* v_ctx,
                          float size)
{
  if (g_opts.debug_mode) {
    log_script_ops_font_size(log_prefix, __func__, log_level_info,
                             size);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  nvgFontSize(p_ctx, size);
}

void script_ops_text_align(void* v_ctx,
                           text_align_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_text_align(log_prefix, __func__, log_level_info,
                              type);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  switch(type) {
  case TEXT_ALIGN_LEFT:
    nvgTextAlignH(p_ctx, NVG_ALIGN_LEFT);
    break;
  case TEXT_ALIGN_CENTER:
    nvgTextAlignH(p_ctx, NVG_ALIGN_CENTER);
    break;
  case TEXT_ALIGN_RIGHT:
    nvgTextAlignH(p_ctx, NVG_ALIGN_RIGHT);
    break;
  }
}

void script_ops_text_base(void* v_ctx,
                          text_base_t type)
{
  if (g_opts.debug_mode) {
    log_script_ops_text_base(log_prefix, __func__, log_level_info,
                             type);
  }

  NVGcontext* p_ctx = (NVGcontext*)v_ctx;
  switch(type) {
  case TEXT_BASE_TOP:
    nvgTextAlignV(p_ctx, NVG_ALIGN_TOP);
    break;
  case TEXT_BASE_MIDDLE:
    nvgTextAlignV(p_ctx, NVG_ALIGN_MIDDLE);
    break;
  case TEXT_BASE_ALPHABETIC:
    nvgTextAlignV(p_ctx, NVG_ALIGN_BASELINE);
    break;
  case TEXT_BASE_BOTTOM:
    nvgTextAlignV(p_ctx, NVG_ALIGN_BOTTOM);
    break;
  }
}
