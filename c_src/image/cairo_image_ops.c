#include <arpa/inet.h>
#include <cairo.h>
#include <stdlib.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "image_ops.h"

uint32_t convert_rgba_to_argb(uint32_t pixel)
{
  return ((pixel & 0xFFFFFF00) >> 8) | ((pixel & 0xFF) << 24);
}

int32_t image_ops_create(void* v_ctx,
                         uint32_t width, uint32_t height,
                         void* p_pixels)
{
  cairo_format_t format = CAIRO_FORMAT_ARGB32;
  int stride = cairo_format_stride_for_width(format, width);
  size_t num_pixels = width * height;
  uint32_t* rgba_pixels = (uint32_t*)p_pixels;
  uint32_t* argb_pixels = malloc(num_pixels * sizeof(uint32_t));

  if (!argb_pixels) return 0;

  for(size_t i = 0; i < num_pixels; ++i) {
    argb_pixels[i] = convert_rgba_to_argb(htonl(rgba_pixels[i]));
  }

  image_data_t* image_data = malloc(sizeof(image_data_t));

  image_data->surface
    = cairo_image_surface_create_for_data((uint8_t*)argb_pixels,
                                          format,
                                          width, height, stride);

  image_data->pattern = cairo_pattern_create_for_surface(image_data->surface);
  cairo_pattern_set_extend(image_data->pattern, CAIRO_EXTEND_REPEAT);

  static cairo_user_data_key_t dummy_key;
  cairo_surface_set_user_data(image_data->surface, &dummy_key, argb_pixels, free);

  return (int32_t)image_data;
}

void image_ops_update(void* v_ctx, uint32_t image_id, void* p_pixels)
{
  image_data_t* image_data = (image_data_t*)image_id;
  uint32_t width = cairo_image_surface_get_width(image_data->surface);
  uint32_t height = cairo_image_surface_get_height(image_data->surface);
  size_t num_pixels = width * height;
  uint32_t* rgba_pixels = (uint32_t*)p_pixels;
  uint32_t* argb_pixels = (uint32_t*)cairo_image_surface_get_data(image_data->surface);

  for(size_t i = 0; i < num_pixels; ++i) {
    argb_pixels[i] = convert_rgba_to_argb(htonl(rgba_pixels[i]));
  }

  cairo_pattern_destroy(image_data->pattern);
  image_data->pattern = cairo_pattern_create_for_surface(image_data->surface);
  cairo_pattern_set_extend(image_data->pattern, CAIRO_EXTEND_REPEAT);
}

void image_ops_delete(void* v_ctx, uint32_t image_id)
{
  image_data_t* image_data = (image_data_t*)image_id;
  cairo_surface_destroy(image_data->surface);
  cairo_pattern_destroy(image_data->pattern);
  free(image_data);
}
