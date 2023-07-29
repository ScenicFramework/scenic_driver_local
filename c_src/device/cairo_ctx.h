#pragma once

#include <cairo.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ops/script_ops.h"

typedef struct {
  color_rgba_t clear_color;
  FT_Library ft_library;
  float font_size;
  text_align_t text_align;
  text_base_t text_base;
  cairo_surface_t* surface;
  cairo_t* cr;
} scenic_cairo_ctx_t;

typedef struct {
  cairo_surface_t* surface;
  cairo_pattern_t* pattern;
} image_data_t;
