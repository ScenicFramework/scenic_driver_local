#pragma once
#include "scenic_types.h"

typedef enum {
  script_op_draw_line = 0x01,
  script_op_draw_triangle = 0x02,
  script_op_draw_quad = 0x03,
  script_op_draw_rect = 0x04,
  script_op_draw_rrect = 0x05,
  script_op_draw_arc = 0x06,
  script_op_draw_sector = 0x07,
  script_op_draw_circle = 0x08,
  script_op_draw_ellipse = 0x09,
  script_op_draw_text = 0x0A,
  script_op_draw_sprites = 0x0B,
  script_op_draw_script = 0x0F,

  script_op_begin_path = 0x20,
  script_op_close_path = 0x21,
  script_op_fill_path = 0x22,
  script_op_stroke_path = 0x23,
  script_op_move_to = 0x26,
  script_op_line_to = 0x27,
  script_op_arc_to = 0x28,
  script_op_bezier_to = 0x29,
  script_op_quadratic_to = 0x2A,
  //script_op_triangle = 0x2B,
  //script_op_quad = 0x2C,
  //script_op_rect = 0x2D,
  //script_op_rrect = 0x2E,
  //script_op_sector = 0x2F,
  //script_op_circle = 0x30,
  //script_op_ellipse = 0x31,

  script_op_push_state = 0x40,
  script_op_pop_state = 0x41,
  script_op_pop_push_state = 0x42,
  script_op_scissor = 0x44,

  script_op_transform = 0x50,
  script_op_scale = 0x51,
  script_op_rotate = 0x52,
  script_op_translate = 0x53,

  script_op_fill_color = 0x60,
  script_op_fill_linear = 0x61,
  script_op_fill_radial = 0x62,
  script_op_fill_image = 0x63,
  script_op_fill_stream = 0x64,

  script_op_stroke_width = 0x70,
  script_op_stroke_color = 0x71,
  script_op_stroke_linear = 0x72,
  script_op_stroke_radial = 0x73,
  script_op_stroke_image = 0x74,
  script_op_stroke_stream = 0x75,

  script_op_line_cap = 0x80,
  script_op_line_join = 0x81,
  script_op_miter_limit = 0x82,

  script_op_font = 0x90,
  script_op_font_size = 0x91,
  script_op_text_align = 0x92,
  script_op_text_base = 0x93,
} script_op_t;

const char* script_op_to_string(script_op_t op);

typedef struct {
  float x;
  float y;
} coordinates_t;

typedef struct {
  float sx;
  float sy;
  float sw;
  float sh;
  float dx;
  float dy;
  float dw;
  float dh;
} sprite_t;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
} color_rgba_t;

typedef enum {
  line_cap_butt = 0x00,
  line_cap_round = 0x01,
  line_cap_square = 0x02,
} line_cap_t;

const char* line_cap_to_string(line_cap_t type);

typedef enum {
  line_join_bevel = 0x00,
  line_join_round = 0x01,
  line_join_miter = 0x02,
} line_join_t;

const char* line_join_to_string(line_join_t type);

typedef enum {
  text_align_left = 0x00,
  text_align_center = 0x01,
  text_align_right = 0x02,
} text_align_t;

const char* text_align_to_string(text_align_t type);

typedef enum {
  text_base_top = 0x00,
  text_base_middle = 0x01,
  text_base_alphabetic = 0x02,
  text_base_bottom = 0x03,
} text_base_t;

const char* text_base_to_string(text_base_t type);

typedef enum {
  flag_fill = 0x01,
  flag_stroke = 0x02,
} fill_stroke_t;

#define SCRIPT_FUNC(name, args...) \
  void script_ops_ ## name(void* v_ctx, ##args); \
  void log_script_ops_ ## name(const char* prefix, const char* func, log_level_t level, ##args);

SCRIPT_FUNC(draw_line, coordinates_t a, coordinates_t b, bool stroke);
SCRIPT_FUNC(draw_triangle, coordinates_t a, coordinates_t b, coordinates_t c, bool fill, bool stroke);
SCRIPT_FUNC(draw_quad, coordinates_t a, coordinates_t b, coordinates_t c, coordinates_t d, bool fill, bool stroke);
SCRIPT_FUNC(draw_rect, float w, float h, bool fill, bool stroke);
SCRIPT_FUNC(draw_rrect, float w, float h, float radius, bool fill, bool stroke);
SCRIPT_FUNC(draw_arc, float radius, float radians, bool fill, bool stroke);
SCRIPT_FUNC(draw_sector, float radius, float radians, bool fill, bool stroke);
SCRIPT_FUNC(draw_circle, float radius, bool fill, bool stroke);
SCRIPT_FUNC(draw_ellipse, float radius0, float radius1, bool fill, bool stroke);
SCRIPT_FUNC(draw_text, uint32_t size, const char* text);
SCRIPT_FUNC(draw_sprites, sid_t id, uint32_t count, const sprite_t* sprites);
SCRIPT_FUNC(draw_script, sid_t id);

SCRIPT_FUNC(begin_path);
SCRIPT_FUNC(close_path);
SCRIPT_FUNC(fill_path);
SCRIPT_FUNC(stroke_path);
SCRIPT_FUNC(move_to, coordinates_t a);
SCRIPT_FUNC(line_to, coordinates_t a);
SCRIPT_FUNC(arc_to, coordinates_t a, coordinates_t b, float radius);
SCRIPT_FUNC(bezier_to, coordinates_t c0, coordinates_t c1, coordinates_t a);
SCRIPT_FUNC(quadratic_to, coordinates_t c, coordinates_t a);

SCRIPT_FUNC(push_state);
SCRIPT_FUNC(pop_state);
SCRIPT_FUNC(scissor, float w, float h);

SCRIPT_FUNC(transform, float a, float b, float c, float d, float e, float f);
SCRIPT_FUNC(scale, float x, float y);
SCRIPT_FUNC(rotate, float radians);
SCRIPT_FUNC(translate, float x, float y);

SCRIPT_FUNC(fill_color, color_rgba_t color);
SCRIPT_FUNC(fill_linear, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end);
SCRIPT_FUNC(fill_radial, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end);
SCRIPT_FUNC(fill_image, sid_t id);
SCRIPT_FUNC(fill_stream, sid_t id);

SCRIPT_FUNC(stroke_width, float w);
SCRIPT_FUNC(stroke_color, color_rgba_t color);
SCRIPT_FUNC(stroke_linear, coordinates_t start, coordinates_t end, color_rgba_t color_start, color_rgba_t color_end);
SCRIPT_FUNC(stroke_radial, coordinates_t center, float inner_radius, float outer_radius, color_rgba_t color_start, color_rgba_t color_end);
SCRIPT_FUNC(stroke_image, sid_t id);
SCRIPT_FUNC(stroke_stream, sid_t id);

SCRIPT_FUNC(line_cap, line_cap_t type);
SCRIPT_FUNC(line_join, line_join_t type);
SCRIPT_FUNC(miter_limit, uint32_t limit);

SCRIPT_FUNC(font, sid_t id);
SCRIPT_FUNC(font_size, float size);
SCRIPT_FUNC(text_align, text_align_t type);
SCRIPT_FUNC(text_base, text_base_t type);
