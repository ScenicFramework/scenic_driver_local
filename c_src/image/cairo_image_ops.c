#include <arpa/inet.h>
#include <cairo.h>
#include <stdlib.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "image_ops.h"

static int maxi(int a, int b) { return a > b ? a : b; }

static
image_pattern_data_t* alloc_image_pattern(scenic_cairo_ctx_t* p_ctx)
{
  image_pattern_data_t* ipd = NULL;
  int i;

  for (i = 0; i < p_ctx->images_used; i++) {
    if (p_ctx->images[i].id == 0) {
      ipd = &p_ctx->images[i];
      break;
    }
  }

  if (ipd == NULL) {
    if (p_ctx->images_used + 1 > p_ctx->images_count) {
      image_pattern_data_t* images;
      int images_count = maxi(p_ctx->images_count + 1, 4) + p_ctx->images_used / 2; // 1.5x over allocation
      images = (image_pattern_data_t*)realloc(p_ctx->images, sizeof(image_pattern_data_t) * images_count);
      if (images == NULL) return NULL;
      p_ctx->images = images;
      p_ctx->images_count = images_count;
    }
    ipd = &p_ctx->images[p_ctx->images_used++];
  }

  memset(ipd, 0, sizeof(*ipd));
  ipd->id = ++p_ctx->highest_image_id;

  return ipd;
}

image_pattern_data_t* find_image_pattern(scenic_cairo_ctx_t* p_ctx, int id)
{
  int i;
  for (i = 0; i < p_ctx->images_used; i++) {
    if (p_ctx->images[i].id == id) {
      return &p_ctx->images[i];
    }
  }
  return NULL;
}

static
int delete_image_pattern(scenic_cairo_ctx_t* p_ctx, int id)
{
  int i;
  for (i = 0; i < p_ctx->images_used; i++) {
    if (p_ctx->images[i].id == id) {
      memset(&p_ctx->images[i], 0, sizeof(p_ctx->images[i]));
      return 1;
    }
  }
  return 0;
}

static
uint32_t convert_rgba_to_argb(uint32_t pixel)
{
  return ((pixel & 0xFFFFFF00) >> 8) | ((pixel & 0xFF) << 24);
}

int32_t image_ops_create(void* v_ctx,
                         uint32_t width, uint32_t height,
                         void* p_pixels)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;

  cairo_format_t format = CAIRO_FORMAT_ARGB32;
  int stride = cairo_format_stride_for_width(format, width);
  size_t num_pixels = width * height;
  uint32_t* rgba_pixels = (uint32_t*)p_pixels;
  uint32_t* argb_pixels = malloc(num_pixels * sizeof(uint32_t));

  if (!argb_pixels) return 0;

  for(size_t i = 0; i < num_pixels; ++i) {
    argb_pixels[i] = convert_rgba_to_argb(htonl(rgba_pixels[i]));
  }

  image_pattern_data_t* image_data = alloc_image_pattern(p_ctx);
  if (!image_data) return 0;

  image_data->surface
    = cairo_image_surface_create_for_data((uint8_t*)argb_pixels,
                                          format,
                                          width, height, stride);

  image_data->pattern = cairo_pattern_create_for_surface(image_data->surface);
  cairo_pattern_set_extend(image_data->pattern, CAIRO_EXTEND_REPEAT);

  static cairo_user_data_key_t dummy_key;
  cairo_surface_set_user_data(image_data->surface, &dummy_key, argb_pixels, free);

  return image_data->id;
}

void image_ops_update(void* v_ctx, int32_t image_id, void* p_pixels)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  image_pattern_data_t* image_data = find_image_pattern(p_ctx, image_id);
  if (!image_data) return;

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

void image_ops_delete(void* v_ctx, int32_t image_id)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)v_ctx;
  image_pattern_data_t* image_data = find_image_pattern(p_ctx, image_id);
  if (!image_data) return;

  cairo_surface_destroy(image_data->surface);
  cairo_pattern_destroy(image_data->pattern);
  delete_image_pattern(p_ctx, image_id);
}
