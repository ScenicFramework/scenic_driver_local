#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "device.h"
#include "fontstash.h"
#include "ops/script_ops.h"

const char* device = "/dev/fb0";

typedef struct {
  int fd;

  struct fb_var_screeninfo var;
  struct fb_fix_screeninfo fix;
} cairo_fb_t;

cairo_fb_t g_cairo_fb = {0};

extern device_info_t g_device_info;
extern device_opts_t g_opts;

int device_init(const device_opts_t* p_opts,
                device_info_t* p_info,
                driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  if ((g_cairo_fb.fd = open(device, O_RDWR)) == -1) {
    log_error("Failed to open device %s: %s", device, strerror(errno));
    return -1;
  }

  if (ioctl(g_cairo_fb.fd, FBIOGET_VSCREENINFO, &g_cairo_fb.var)) {
    log_error("Failed to get fb_var_screeninfo: %s", strerror(errno));
    return -1;
  }

  if (ioctl(g_cairo_fb.fd, FBIOGET_FSCREENINFO, &g_cairo_fb.fix)) {
    log_error("Failed to get fb_fix_screeninfo: %s", strerror(errno));
    return -1;
  }

  scenic_cairo_ctx_t* p_ctx = calloc(1, sizeof(scenic_cairo_ctx_t));

  FT_Error status = FT_Init_FreeType(&p_ctx->ft_library);
  if (status != 0) {
    log_error("cairo: FT_Init_FreeType: Error: %d", status);
    close(g_cairo_fb.fd);
    free(p_ctx);

    return -1;
  }

  p_info->width = g_cairo_fb.var.xres;
  p_info->height = g_cairo_fb.var.yres;

  p_ctx->font_size = 10.0; // Cairo default
  p_ctx->text_align = TEXT_ALIGN_LEFT;
  p_ctx->text_base = TEXT_BASE_ALPHABETIC;

  p_ctx->clear_color = (color_rgba_t){
    // black opaque
    .red = 0.0,
    .green = 0.0,
    .blue = 0.0,
    .alpha = 1.0
  };

  p_info->v_ctx = p_ctx;

  p_ctx->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                              p_info->width, p_info->height);
  return 0;
}

int device_close(device_info_t* p_info)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }
  close(g_cairo_fb.fd);

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_info->v_ctx;
  cairo_surface_destroy(p_ctx->surface);
  free(p_ctx);
}

void device_poll()
{
}

void device_begin_render(driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;

  p_ctx->cr = cairo_create(p_ctx->surface);

  // Paint surface to clear color
  cairo_set_source_rgba(p_ctx->cr,
                        p_ctx->clear_color.red,
                        p_ctx->clear_color.green,
                        p_ctx->clear_color.blue,
                        p_ctx->clear_color.alpha);
  cairo_paint(p_ctx->cr);
}

void device_begin_cursor_render(driver_data_t* p_data)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;
  cairo_translate(p_ctx->cr, p_data->cursor_pos[0], p_data->cursor_pos[1]);
}

void render_cairo_surface_to_fb(int fd, cairo_surface_t* surface)
{
  cairo_surface_flush(surface);
  uint8_t* surface_data = cairo_image_surface_get_data(surface);

  int fb_x = g_cairo_fb.var.xres;
  int fb_y = g_cairo_fb.var.yres;
  size_t fb_size = fb_x * fb_y * 3;
  uint8_t* fbbuff = mmap(NULL, fb_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

  for(uint32_t i = 0, j = 0; i < fb_size; i += 3, j += 4) {
    fbbuff[i+0] = surface_data[j+0];
    fbbuff[i+1] = surface_data[j+1];
    fbbuff[i+2] = surface_data[j+2];
  }

  munmap(fbbuff, fb_size);
}

void device_end_render(driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;
  render_cairo_surface_to_fb(g_cairo_fb.fd, p_ctx->surface);

  cairo_destroy(p_ctx->cr);
}

void device_clear_color(float red, float green, float blue, float alpha)
{
  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)g_device_info.v_ctx;
  p_ctx->clear_color = (color_rgba_t){
    .red = red,
    .green = green,
    .blue = blue,
    .alpha = alpha
  };
}

char* device_gl_error()
{
  return NULL;
}
