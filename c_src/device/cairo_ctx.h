#pragma once

#include <cairo.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ops/script_ops.h"

typedef struct {
  cairo_pattern_t* fill;
  cairo_pattern_t* stroke;
} fill_stroke_pattern_t;

typedef struct pattern_stack_t_ {
  fill_stroke_pattern_t pattern;
  struct pattern_stack_t_* next;
} pattern_stack_t;

typedef struct {
  color_rgba_t clear_color;
  FT_Library ft_library;
  float font_size;
  text_align_t text_align;
  text_base_t text_base;
  cairo_surface_t* surface;
  cairo_t* cr;
  pattern_stack_t* pattern_stack_head;
  fill_stroke_pattern_t pattern;
} scenic_cairo_ctx_t;

typedef struct {
  cairo_surface_t* surface;
  cairo_pattern_t* pattern;
} image_data_t;

void pattern_stack_push(scenic_cairo_ctx_t* p_ctx);
void pattern_stack_pop(scenic_cairo_ctx_t* p_ctx);
