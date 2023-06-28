#include "comms.h"
#include "script.h"
#include "script_ops.h"

static const char* log_prefix = "Unimplemented";

__attribute__((weak))
void script_ops_draw_line(void* v_ctx, coordinates_t a, coordinates_t b, bool stroke)
{
  log_script_ops_draw_line(log_prefix, __func__, log_level_warn, a, b, stroke);
}
void log_script_ops_draw_line(const char* prefix, const char* func, log_level_t level, coordinates_t a, coordinates_t b, bool stroke)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}, "
              "b: {x:%.1f, y:%.1f}, "
              "stroke: %s"
              "}",
              prefix, func,
              a.x, a.y,
              b.x, b.y,
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_triangle(void* v_ctx, coordinates_t a, coordinates_t b, coordinates_t c, bool fill, bool stroke)
{
  log_script_ops_draw_triangle(log_prefix, __func__, log_level_warn, a, b, c, fill, stroke);
}
void log_script_ops_draw_triangle(const char* prefix, const char* func, log_level_t level, coordinates_t a, coordinates_t b, coordinates_t c, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}, "
              "b: {x:%.1f, y:%.1f}, "
              "c: {x:%.1f, y:%.1f}, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              a.x, a.y,
              b.x, b.y,
              c.x, c.y,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_quad(void* v_ctx, coordinates_t a, coordinates_t b, coordinates_t c, coordinates_t d, bool fill, bool stroke)
{
  log_script_ops_draw_quad(log_prefix, __func__, log_level_warn, a, b, c, d, fill, stroke);
}
void log_script_ops_draw_quad(const char* prefix, const char* func, log_level_t level, coordinates_t a, coordinates_t b, coordinates_t c, coordinates_t d, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}, "
              "b: {x:%.1f, y:%.1f}, "
              "c: {x:%.1f, y:%.1f}, "
              "d: {x:%.1f, y:%.1f}, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              a.x, a.y,
              b.x, b.y,
              c.x, c.y,
              d.x, d.y,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_rect(void* v_ctx, float w, float h, bool fill, bool stroke)
{
  log_script_ops_draw_rect(log_prefix, __func__, log_level_warn, w, h, fill, stroke);
}
void log_script_ops_draw_rect(const char* prefix, const char* func, log_level_t level, float w, float h, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
             "w: %.1f, "
              "h: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              w,
              h,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_rrect(void* v_ctx, float w, float h, float radius, bool fill, bool stroke)
{
  log_script_ops_draw_rrect(log_prefix, __func__, log_level_warn, w, h, radius, fill, stroke);
}
void log_script_ops_draw_rrect(const char* prefix, const char* func, log_level_t level, float w, float h, float radius, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "w: %.1f, "
              "h: %.1f, "
              "radius: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              w,
              h,
              radius,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_arc(void* v_ctx, float radius, float radians, bool fill, bool stroke)
{
  log_script_ops_draw_arc(log_prefix, __func__, log_level_warn, radius, radians, fill, stroke);
}
void log_script_ops_draw_arc(const char* prefix, const char* func, log_level_t level, float radius, float radians, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "radius: %.1f, "
              "radians: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              radius,
              radians,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_sector(void* v_ctx, float radius, float radians, bool fill, bool stroke)
{
  log_script_ops_draw_sector(log_prefix, __func__, log_level_warn, radius, radians, fill, stroke);
}
void log_script_ops_draw_sector(const char* prefix, const char* func, log_level_t level, float radius, float radians, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "radius: %.1f, "
              "radians: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              radius,
              radians,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_circle(void* v_ctx, float radius, bool fill, bool stroke)
{
  log_script_ops_draw_circle(log_prefix, __func__, log_level_warn, radius, fill, stroke);
}
void log_script_ops_draw_circle(const char* prefix, const char* func, log_level_t level, float radius, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "radius: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              radius,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_ellipse(void* v_ctx, float radius0, float radius1, bool fill, bool stroke)
{
  log_script_ops_draw_ellipse(log_prefix, __func__, log_level_warn, radius0, radius1, fill, stroke);
}
void log_script_ops_draw_ellipse(const char* prefix, const char* func, log_level_t level, float radius0, float radius1, bool fill, bool stroke)
{
  log_message(level, "%s %s: %{"
              "radius0: %.1f, "
              "radius1: %.1f, "
              "fill: %s, "
              "stroke: %s"
              "}",
              prefix, func,
              radius0,
              radius1,
              (fill) ? "true" : "false",
              (stroke) ? "true" : "false");
}

__attribute__((weak))
void script_ops_draw_text(void* v_ctx, uint32_t size, const char* text)
{
  log_script_ops_draw_text(log_prefix, __func__, log_level_warn, size, text);
}
void log_script_ops_draw_text(const char* prefix, const char* func, log_level_t level, uint32_t size, const char* text)
{
  log_message(level, "%s %s: %{"
              "size: %d, "
              "text: '%s'"
              "}",
              prefix, func,
              size,
              text);
}

__attribute__((weak))
void script_ops_draw_sprites(void* v_ctx, sid_t id, uint32_t count, const sprite_t* sprites)
{
  log_script_ops_draw_sprites(log_prefix, __func__, log_level_warn, id, count, sprites);
}
void log_script_ops_draw_sprites(const char* prefix, const char* func, log_level_t level, sid_t id, uint32_t count, const sprite_t* sprites)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_draw_script(void* v_ctx, sid_t id)
{
  render_script(v_ctx, id);
}

__attribute__((weak))
void script_ops_begin_path(void* v_ctx)
{
  log_script_ops_begin_path(log_prefix, __func__, log_level_warn);
}
void log_script_ops_begin_path(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_close_path(void* v_ctx)
{
  log_script_ops_close_path(log_prefix, __func__, log_level_warn);
}
void log_script_ops_close_path(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_fill_path(void* v_ctx)
{
  log_script_ops_fill_path(log_prefix, __func__, log_level_warn);
}
void log_script_ops_fill_path(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_stroke_path(void* v_ctx)
{
  log_script_ops_stroke_path(log_prefix, __func__, log_level_warn);
}
void log_script_ops_stroke_path(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_move_to(void* v_ctx, coordinates_t a)
{
  log_script_ops_move_to(log_prefix, __func__, log_level_warn, a);
}
void log_script_ops_move_to(const char* prefix, const char* func, log_level_t level, coordinates_t a)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}"
              "}",
              prefix, func,
              a.x, a.y);
}

__attribute__((weak))
void script_ops_line_to(void* v_ctx, coordinates_t a)
{
  log_script_ops_line_to(log_prefix, __func__, log_level_warn, a);
}
void log_script_ops_line_to(const char* prefix, const char* func, log_level_t level, coordinates_t a)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}"
              "}",
              prefix, func,
              a.x, a.y);
}

__attribute__((weak))
void script_ops_arc_to(void* v_ctx, coordinates_t a, coordinates_t b, float radius)
{
  log_script_ops_arc_to(log_prefix, __func__, log_level_warn, a, b, radius);
}
void log_script_ops_arc_to(const char* prefix, const char* func, log_level_t level, coordinates_t a, coordinates_t b, float radius)
{
  log_message(level, "%s %s: %{"
              "a: {x:%.1f, y:%.1f}, "
              "b: {x:%.1f, y:%.1f}, "
              "radius: %.1f"
              "}",
              prefix, func,
              a.x, a.y,
              b.x, b.y,
              radius);
}

__attribute__((weak))
void script_ops_bezier_to(void* v_ctx, coordinates_t c0, coordinates_t c1, coordinates_t a)
{
  log_script_ops_bezier_to(log_prefix, __func__, log_level_warn, c0, c1, a);
}
void log_script_ops_bezier_to(const char* prefix, const char* func, log_level_t level, coordinates_t c0, coordinates_t c1, coordinates_t a)
{
  log_message(level, "%s %s: %{"
              "c0: {x:%.1f, y:%.1f}, "
              "c1: {x:%.1f, y:%.1f}, "
              "a: {x:%.1f, y:%.1f}"
              "}",
              prefix, func,
              c0.x, c0.y,
              c1.x, c1.y,
              a.x, a.y);
}

__attribute__((weak))
void script_ops_quadratic_to(void* v_ctx, coordinates_t c, coordinates_t a)
{
  log_script_ops_quadratic_to(log_prefix, __func__, log_level_warn, c, a);
}
void log_script_ops_quadratic_to(const char* prefix, const char* func, log_level_t level, coordinates_t c, coordinates_t a)
{
  log_message(level, "%s %s: %{"
              "c: {x:%.1f, y:%.1f}, "
              "a: {x:%.1f, y:%.1f}"
              "}",
              prefix, func,
              c.x, c.y,
              a.x, a.y);
}

__attribute__((weak))
void script_ops_push_state(void* v_ctx)
{
  log_script_ops_push_state(log_prefix, __func__, log_level_warn);
}
void log_script_ops_push_state(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_pop_state(void* v_ctx)
{
  log_script_ops_pop_state(log_prefix, __func__, log_level_warn);
}
void log_script_ops_pop_state(const char* prefix, const char* func, log_level_t level)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_scissor(void* v_ctx, float w, float h)
{
  log_script_ops_scissor(log_prefix, __func__, log_level_warn, w, h);
}
void log_script_ops_scissor(const char* prefix, const char* func, log_level_t level, float w, float h)
{
  log_message(level, "%s %s: %{"
              "w: %.1f, "
              "h: %.1f"
              "}",
              prefix, func,
              w,
              h);
}

__attribute__((weak))
void script_ops_transform(void* v_ctx, float a, float b, float c, float d, float e, float f)
{
  log_script_ops_transform(log_prefix, __func__, log_level_warn, a, b, c, d, e, f);
}
void log_script_ops_transform(const char* prefix, const char* func, log_level_t level, float a, float b, float c, float d, float e, float f)
{
  log_message(level, "%s %s: %{"
              "a: %.1f, "
              "b: %.1f, "
              "c: %.1f, "
              "d: %.1f, "
              "e: %.1f, "
              "f: %.1f"
              "}",
              prefix, func,
              a,
              b,
              c,
              d,
              e,
              f);
}

__attribute__((weak))
void script_ops_scale(void* v_ctx, float x, float y)
{
  log_script_ops_scale(log_prefix, __func__, log_level_warn, x, y);
}
void log_script_ops_scale(const char* prefix, const char* func, log_level_t level, float x, float y)
{
  log_message(level, "%s %s: %{"
              "x: %.1f, "
              "y: %.1f"
              "}",
              prefix, func,
              x,
              y);
}

__attribute__((weak))
void script_ops_rotate(void* v_ctx, float radians)
{
  log_script_ops_rotate(log_prefix, __func__, log_level_warn, radians);
}
void log_script_ops_rotate(const char* prefix, const char* func, log_level_t level, float radians)
{
  log_message(level, "%s %s: %{"
              "radians: %.1f"
              "}",
              prefix, func,
              radians);
}

__attribute__((weak))
void script_ops_translate(void* v_ctx, float x, float y)
{
  log_script_ops_translate(log_prefix, __func__, log_level_warn, x, y);
}
void log_script_ops_translate(const char* prefix, const char* func, log_level_t level, float x, float y)
{
  log_message(level, "%s %s: %{"
              "x: %.1f, "
              "y: %.1f"
              "}",
              prefix, func,
              x,
              y);
}

__attribute__((weak))
void script_ops_fill_color(void* v_ctx, color_rgba_t color)
{
  log_script_ops_fill_color(log_prefix, __func__, log_level_warn, color);
}
void log_script_ops_fill_color(const char* prefix, const char* func, log_level_t level, color_rgba_t color)
{
  log_message(level, "%s %s: %{"
              "color: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              color.red, color.green, color.blue, color.alpha);
}

__attribute__((weak))
void script_ops_fill_linear(void* v_ctx, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end)
{
  log_script_ops_fill_linear(log_prefix, __func__, log_level_warn, start, end, color_start, color_end);
}
void log_script_ops_fill_linear(const char* prefix, const char* func, log_level_t level, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end)
{
  log_message(level, "%s %s: %{"
              "start: {x:%.1f, y:%.1f}, "
              "end: {x:%.1f, y:%.1f}, "
              "color_start: {r:%02x, g:%02x, b:%02x, a:%02x}, "
              "color_end: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              start.x, start.y,
              end.x, end.y,
              color_start.red, color_start.green, color_start.blue, color_start.alpha,
              color_end.red, color_end.green, color_end.blue, color_end.alpha);
}

__attribute__((weak))
void script_ops_fill_radial(void* v_ctx, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end)
{
  log_script_ops_fill_radial(log_prefix, __func__, log_level_warn, center, inner_radius, outer_radius, color_start, color_end);
}
void log_script_ops_fill_radial(const char* prefix, const char* func, log_level_t level, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end)
{
  log_message(level, "%s %s: %{"
              "center: {x:%.1f, y:%.1f}, "
              "inner_radius: %.1f, "
              "outer_radius: %.1f, "
              "color_start: {r:%02x, g:%02x, b:%02x, a:%02x}, "
              "color_end: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              center.x, center.y,
              inner_radius,
              outer_radius,
              color_start.red, color_start.green, color_start.blue, color_start.alpha,
              color_end.red, color_end.green, color_end.blue, color_end.alpha);
}

__attribute__((weak))
void script_ops_fill_image(void* v_ctx, sid_t id)
{
  log_script_ops_fill_image(log_prefix, __func__, log_level_warn, id);
}
void log_script_ops_fill_image(const char* prefix, const char* func, log_level_t level, sid_t id)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_fill_stream(void* v_ctx, sid_t id)
{
  log_script_ops_fill_stream(log_prefix, __func__, log_level_warn, id);
}
void log_script_ops_fill_stream(const char* prefix, const char* func, log_level_t level, sid_t id)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_stroke_width(void* v_ctx, float w)
{
  log_script_ops_stroke_width(log_prefix, __func__, log_level_warn, w);
}
void log_script_ops_stroke_width(const char* prefix, const char* func, log_level_t level, float w)
{
  log_message(level, "%s %s: %{"
              "w: %.1f"
              "}",
              prefix, func,
              w);
}

__attribute__((weak))
void script_ops_stroke_color(void* v_ctx, color_rgba_t color)
{
  log_script_ops_stroke_color(log_prefix, __func__, log_level_warn, color);
}
void log_script_ops_stroke_color(const char* prefix, const char* func, log_level_t level, color_rgba_t color)
{
  log_message(level, "%s %s: %{"
              "color: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              color.red, color.green, color.blue, color.alpha);
}

__attribute__((weak))
void script_ops_stroke_linear(void* v_ctx, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end)
{
  log_script_ops_stroke_linear(log_prefix, __func__, log_level_warn, start, end, color_start, color_end);
}
void log_script_ops_stroke_linear(const char* prefix, const char* func, log_level_t level, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end)
{
  log_message(level, "%s %s: %{"
              "start: {x:%.1f, y:%.1f}, "
              "end: {x:%.1f, y:%.1f}, "
              "color_start: {r:%02x, g:%02x, b:%02x, a:%02x}, "
              "color_end: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              start.x, start.y,
              end.x, end.y,
              color_start.red, color_start.green, color_start.blue, color_start.alpha,
              color_end.red, color_end.green, color_end.blue, color_end.alpha);
}

__attribute__((weak))
void script_ops_stroke_radial(void* v_ctx, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end)
{
  log_script_ops_stroke_radial(log_prefix, __func__, log_level_warn, center, inner_radius, outer_radius, color_start, color_end);
}
void log_script_ops_stroke_radial(const char* prefix, const char* func, log_level_t level, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end)
{
  log_message(level, "%s %s: %{"
              "center: {x:%.1f, y:%.1f}, "
              "inner_radius: %.1f, "
              "outer_radius: %.1f, "
              "color_start: {r:%02x, g:%02x, b:%02x, a:%02x}, "
              "color_end: {r:%02x, g:%02x, b:%02x, a:%02x}"
              "}",
              prefix, func,
              center.x, center.y,
              inner_radius,
              outer_radius,
              color_start.red, color_start.green, color_start.blue, color_start.alpha,
              color_end.red, color_end.green, color_end.blue, color_end.alpha);
}

__attribute__((weak))
void script_ops_stroke_image(void* v_ctx, sid_t id)
{
  log_script_ops_stroke_image(log_prefix, __func__, log_level_warn, id);
}
void log_script_ops_stroke_image(const char* prefix, const char* func, log_level_t level, sid_t id)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_stroke_stream(void* v_ctx, sid_t id)
{
  log_script_ops_stroke_stream(log_prefix, __func__, log_level_warn, id);
}
void log_script_ops_stroke_stream(const char* prefix, const char* func, log_level_t level, sid_t id)
{
  log_message(level, "%s %s:", __func__);
}

const char* line_cap_to_string(line_cap_t type)
{
  switch(type) {
  case line_cap_butt: return "butt";
  case line_cap_round: return "round";
  case line_cap_square: return "square";
  }

  return NULL;
}
__attribute__((weak))
void script_ops_line_cap(void* v_ctx, line_cap_t type)
{
  log_script_ops_line_cap(log_prefix, __func__, log_level_warn, type);
}
void log_script_ops_line_cap(const char* prefix, const char* func, log_level_t level, line_cap_t type)
{
  log_message(level, "%s %s: %{"
              "type: %s"
              "}",
              prefix, func,
              line_cap_to_string(type));
}

const char* line_join_to_string(line_join_t type)
{
  switch(type) {
  case line_join_bevel: return "bevel";
  case line_join_round: return "round";
  case line_join_miter: return "miter";
  }

  return NULL;
}
__attribute__((weak))
void script_ops_line_join(void* v_ctx, line_join_t type)
{
  log_script_ops_line_join(log_prefix, __func__, log_level_warn, type);
}
void log_script_ops_line_join(const char* prefix, const char* func, log_level_t level, line_join_t type)
{
  log_message(level, "%s %s: %{"
              "type: %s"
              "}",
              prefix, func,
              line_join_to_string(type));
}

__attribute__((weak))
void script_ops_miter_limit(void* v_ctx, uint32_t limit)
{
  log_script_ops_miter_limit(log_prefix, __func__, log_level_warn, limit);
}
void log_script_ops_miter_limit(const char* prefix, const char* func, log_level_t level, uint32_t limit)
{
  log_message(level, "%s %s: %{"
              "limit: %d"
              "}",
              prefix, func,
              limit);
}

__attribute__((weak))
void script_ops_font(void* v_ctx, sid_t id)
{
  log_script_ops_font(log_prefix, __func__, log_level_warn, id);
}
void log_script_ops_font(const char* prefix, const char* func, log_level_t level, sid_t id)
{
  log_message(level, "%s %s", prefix, func);
}

__attribute__((weak))
void script_ops_font_size(void* v_ctx, float size)
{
  log_script_ops_font_size(log_prefix, __func__, log_level_warn, size);
}
void log_script_ops_font_size(const char* prefix, const char* func, log_level_t level, float size)
{
  log_message(level, "%s %s: %{"
              "size: %.1f"
              "}",
              prefix, func,
              size);
}

const char* text_align_to_string(text_align_t type)
{
  switch(type) {
  case text_align_left: return "left";
  case text_align_center: return "center";
  case text_align_right: return "right";
  }

  return NULL;
}
__attribute__((weak))
void script_ops_text_align(void* v_ctx, text_align_t type)
{
  log_script_ops_text_align(log_prefix, __func__, log_level_warn, type);
}
void log_script_ops_text_align(const char* prefix, const char* func, log_level_t level, text_align_t type)
{
  log_message(level, "%s %s: %{"
              "type: %s"
              "}",
              prefix, func,
              text_align_to_string(type));
}

const char* text_base_to_string(text_base_t type)
{
  switch(type) {
  case text_base_top: return "top";
  case text_base_middle: return "middle";
  case text_base_alphabetic: return "alphabetic";
  case text_base_bottom: return "bottom";
  }

  return NULL;
}
__attribute__((weak))
void script_ops_text_base(void* v_ctx, text_base_t type)
{
  log_script_ops_text_base(log_prefix, __func__, log_level_warn, type);
}
void log_script_ops_text_base(const char* prefix, const char* func, log_level_t level, text_base_t type)
{
  log_message(level, "%s %s: %{"
              "type: %s"
              "}",
              prefix, func,
              text_base_to_string(type));
}

const char* script_op_to_string(script_op_t op)
{
  switch(op) {
  case script_op_draw_line: return "script_op_draw_line";
  case script_op_draw_triangle: return "script_op_draw_triangle";
  case script_op_draw_quad: return "script_op_draw_quad";
  case script_op_draw_rect: return "script_op_draw_rect";
  case script_op_draw_rrect: return "script_op_draw_rrect";
  case script_op_draw_arc: return "script_op_draw_arc";
  case script_op_draw_sector: return "script_op_draw_sector";
  case script_op_draw_circle: return "script_op_draw_circle";
  case script_op_draw_ellipse: return "script_op_draw_ellipse";
  case script_op_draw_text: return "script_op_draw_text";
  case script_op_draw_sprites: return "script_op_draw_sprites";
  case script_op_draw_script: return "script_op_draw_script";

  case script_op_begin_path: return "script_op_begin_path";
  case script_op_close_path: return "script_op_close_path";
  case script_op_fill_path: return "script_op_fill_path";
  case script_op_stroke_path: return "script_op_stroke_path";
  case script_op_move_to: return "script_op_move_to";
  case script_op_line_to: return "script_op_line_to";
  case script_op_arc_to: return "script_op_arc_to";
  case script_op_bezier_to: return "script_op_bezier_to";
  case script_op_quadratic_to: return "script_op_quadratic_to";

  case script_op_push_state: return "script_op_push_state";
  case script_op_pop_state: return "script_op_pop_state";
  case script_op_pop_push_state: return "script_op_pop_push_state";
  case script_op_scissor: return "script_op_scissor";

  case script_op_transform: return "script_op_transform";
  case script_op_scale: return "script_op_scale";
  case script_op_rotate: return "script_op_rotate";
  case script_op_translate: return "script_op_translate";

  case script_op_fill_color: return "script_op_fill_color";
  case script_op_fill_linear: return "script_op_fill_linear";
  case script_op_fill_radial: return "script_op_fill_radial";
  case script_op_fill_image: return "script_op_fill_image";
  case script_op_fill_stream: return "script_op_fill_stream";

  case script_op_stroke_width: return "script_op_stroke_width";
  case script_op_stroke_color: return "script_op_stroke_color";
  case script_op_stroke_linear: return "script_op_stroke_linear";
  case script_op_stroke_radial: return "script_op_stroke_radial";
  case script_op_stroke_image: return "script_op_stroke_image";
  case script_op_stroke_stream: return "script_op_stroke_stream";

  case script_op_line_cap: return "script_op_line_cap";
  case script_op_line_join: return "script_op_line_join";
  case script_op_miter_limit: return "script_op_miter_limit";

  case script_op_font: return "script_op_font";
  case script_op_font_size: return "script_op_font_size";
  case script_op_text_align: return "script_op_text_align";
  case script_op_text_base: return "script_op_text_base";
  }

  return NULL;
}
