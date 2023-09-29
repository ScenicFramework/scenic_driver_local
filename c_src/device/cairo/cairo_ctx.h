#pragma once

#include <cairo.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "script_ops.h"

typedef struct {
  cairo_pattern_t* fill;
  cairo_pattern_t* stroke;
} fill_stroke_pattern_t;

typedef struct pattern_stack_t_ {
  fill_stroke_pattern_t pattern;
  struct pattern_stack_t_* next;
} pattern_stack_t;

typedef struct {
  int id;
  cairo_surface_t* surface;
  cairo_pattern_t* pattern;
} image_pattern_data_t;

typedef struct {
  int id;
  cairo_font_face_t* font_face;
} font_data_t;

typedef struct {
  color_rgba_t clear_color;
  FT_Library ft_library;
  float font_size;
  text_align_t text_align;
  text_base_t text_base;
  union {
    u_int8_t  *c;
    u_int16_t *s;
    u_int32_t *i;
  } fbbuff;
  cairo_surface_t* surface;
  cairo_t* cr;
  pattern_stack_t* pattern_stack_head;
  fill_stroke_pattern_t pattern;
  int images_count;
  int images_used;
  int highest_image_id;
  image_pattern_data_t* images;
  int fonts_count;
  int fonts_used;
  int highest_font_id;
  font_data_t* fonts;
  float dist_tolerance;
  float ratio;
} scenic_cairo_ctx_t;

void pattern_stack_push(scenic_cairo_ctx_t* p_ctx);
void pattern_stack_pop(scenic_cairo_ctx_t* p_ctx);

image_pattern_data_t* find_image_pattern(scenic_cairo_ctx_t* p_ctx, int id);
font_data_t* find_font(scenic_cairo_ctx_t* p_ctx, int id);
