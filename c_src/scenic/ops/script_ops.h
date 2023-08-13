#pragma once
#include "comms.h"
#include "scenic_types.h"

typedef enum {
  SCRIPT_OP_DRAW_LINE = 0X01,
  SCRIPT_OP_DRAW_TRIANGLE = 0X02,
  SCRIPT_OP_DRAW_QUAD = 0X03,
  SCRIPT_OP_DRAW_RECT = 0X04,
  SCRIPT_OP_DRAW_RRECT = 0X05,
  SCRIPT_OP_DRAW_ARC = 0X06,
  SCRIPT_OP_DRAW_SECTOR = 0X07,
  SCRIPT_OP_DRAW_CIRCLE = 0X08,
  SCRIPT_OP_DRAW_ELLIPSE = 0X09,
  SCRIPT_OP_DRAW_TEXT = 0X0A,
  SCRIPT_OP_DRAW_SPRITES = 0X0B,
  SCRIPT_OP_DRAW_SCRIPT = 0X0F,

  SCRIPT_OP_BEGIN_PATH = 0X20,
  SCRIPT_OP_CLOSE_PATH = 0X21,
  SCRIPT_OP_FILL_PATH = 0X22,
  SCRIPT_OP_STROKE_PATH = 0X23,
  SCRIPT_OP_MOVE_TO = 0X26,
  SCRIPT_OP_LINE_TO = 0X27,
  SCRIPT_OP_ARC_TO = 0X28,
  SCRIPT_OP_BEZIER_TO = 0X29,
  SCRIPT_OP_QUADRATIC_TO = 0X2A,
  SCRIPT_OP_ARC = 0X32,

  SCRIPT_OP_PUSH_STATE = 0X40,
  SCRIPT_OP_POP_STATE = 0X41,
  SCRIPT_OP_POP_PUSH_STATE = 0X42,
  SCRIPT_OP_SCISSOR = 0X44,

  SCRIPT_OP_TRANSFORM = 0X50,
  SCRIPT_OP_SCALE = 0X51,
  SCRIPT_OP_ROTATE = 0X52,
  SCRIPT_OP_TRANSLATE = 0X53,

  SCRIPT_OP_FILL_COLOR = 0X60,
  SCRIPT_OP_FILL_LINEAR = 0X61,
  SCRIPT_OP_FILL_RADIAL = 0X62,
  SCRIPT_OP_FILL_IMAGE = 0X63,
  SCRIPT_OP_FILL_STREAM = 0X64,

  SCRIPT_OP_STROKE_WIDTH = 0X70,
  SCRIPT_OP_STROKE_COLOR = 0X71,
  SCRIPT_OP_STROKE_LINEAR = 0X72,
  SCRIPT_OP_STROKE_RADIAL = 0X73,
  SCRIPT_OP_STROKE_IMAGE = 0X74,
  SCRIPT_OP_STROKE_STREAM = 0X75,

  SCRIPT_OP_LINE_CAP = 0X80,
  SCRIPT_OP_LINE_JOIN = 0X81,
  SCRIPT_OP_MITER_LIMIT = 0X82,

  SCRIPT_OP_FONT = 0X90,
  SCRIPT_OP_FONT_SIZE = 0X91,
  SCRIPT_OP_TEXT_ALIGN = 0X92,
  SCRIPT_OP_TEXT_BASE = 0X93,
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
  LINE_CAP_BUTT = 0X00,
  LINE_CAP_ROUND = 0X01,
  LINE_CAP_SQUARE = 0X02,
} line_cap_t;

const char* line_cap_to_string(line_cap_t type);

typedef enum {
  LINE_JOIN_BEVEL = 0X00,
  LINE_JOIN_ROUND = 0X01,
  LINE_JOIN_MITER = 0X02,
} line_join_t;

const char* line_join_to_string(line_join_t type);

typedef enum {
  TEXT_ALIGN_LEFT = 0X00,
  TEXT_ALIGN_CENTER = 0X01,
  TEXT_ALIGN_RIGHT = 0X02,
} text_align_t;

const char* text_align_to_string(text_align_t type);

typedef enum {
  TEXT_BASE_TOP = 0X00,
  TEXT_BASE_MIDDLE = 0X01,
  TEXT_BASE_ALPHABETIC = 0X02,
  TEXT_BASE_BOTTOM = 0X03,
} text_base_t;

const char* text_base_to_string(text_base_t type);

typedef enum {
  FLAG_FILL = 0X01,
  FLAG_STROKE = 0X02,
} fill_stroke_t;

typedef enum {
  SWEEP_DIR_CCW = 0X01,
  SWEEP_DIR_CW = 0X02,
} sweep_dir_t;

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
SCRIPT_FUNC(arc, coordinates_t c, float radius, float a0, float a1, sweep_dir_t sweep_dir);

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
