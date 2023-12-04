#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "device.h"
#include "fontstash.h"
#include "scenic_ops.h"

#define FB0_TIMEOUT 60 //seconds
const char* device = "/dev/fb0";

typedef struct {
  int fd;

  union {
    u_int8_t  *c;
    u_int16_t *s;
    u_int32_t *i;
  } rgb_buff;

  struct fb_var_screeninfo var;
  struct fb_fix_screeninfo fix;
} cairo_fb_t;

cairo_fb_t g_cairo_fb = {0};

extern device_info_t g_device_info;
extern device_opts_t g_opts;

// Support for 8 bits per pixel colors
unsigned short red[256], green[256], blue[256];
struct fb_cmap map332 = {0, 256, red, green, blue, NULL};
unsigned short red_b[256], green_b[256], blue_b[256];
struct fb_cmap map_back = {0, 256, red_b, green_b, blue_b, NULL};

void make332map(struct fb_cmap *map)
{
  int rs, gs, bs, i;
  int r = 8, g = 8, b = 4;

  map->red = red;
  map->green = green;
  map->blue = blue;

  rs = 256 / (r - 1);
  gs = 256 / (g - 1);
  bs = 256 / (b - 1);

  for (i = 0; i < 256; i++) {
    map->red[i]   = (rs * ((i / (g * b)) % r)) * 255;
    map->green[i] = (gs * ((i / b) % g)) * 255;
    map->blue[i]  = (bs * ((i) % b)) * 255;
  }
}

void set8map(int fh, struct fb_cmap *map)
{
    if (ioctl(fh, FBIOPUTCMAP, map) < 0) {
        log_error("cairo: Error putting colormap");
    }
}

void get8map(int fh, struct fb_cmap *map)
{
    if (ioctl(fh, FBIOGETCMAP, map) < 0) {
        log_error("cairo: Error getting colormap");
    }
}

void set332map(int fh)
{
    make332map(&map332);
    set8map(fh, &map332);
}

int device_init(const device_opts_t* p_opts,
                device_info_t* p_info,
                driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = scenic_cairo_init(p_opts, p_info);
  if (!p_ctx) {
    return -1;
  }

  p_info->v_ctx = p_ctx;

  time_t fb0_timer_start = time(NULL);
  while ((time(NULL) - fb0_timer_start) < FB0_TIMEOUT) {
    if ((g_cairo_fb.fd = open(device, O_RDWR)) != -1) {
      break;
    }
    sched_yield();
  }

  if (g_cairo_fb.fd == -1) {
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

  uint32_t width = cairo_image_surface_get_width(p_ctx->surface);
  uint32_t height = cairo_image_surface_get_height(p_ctx->surface);
  size_t pix_count = width * height;

  switch (g_cairo_fb.var.bits_per_pixel)
  {
  case 8:
    g_cairo_fb.rgb_buff.c = (uint8_t*)malloc(pix_count * sizeof(uint8_t));

    get8map(g_cairo_fb.fd, &map_back);
    set332map(g_cairo_fb.fd);
    break;
  case 15:
  case 16:
    g_cairo_fb.rgb_buff.c = (uint8_t*)malloc(pix_count * sizeof(uint16_t));
    break;
  case 24:
    g_cairo_fb.rgb_buff.c = (uint8_t*)malloc(pix_count * 3 * sizeof(uint8_t));
    break;
  case 32:
    g_cairo_fb.rgb_buff.c = (uint8_t*)malloc(pix_count * sizeof(uint32_t));
    break;
  default:
    log_error("cairo: Unsupported video mode: %dbpp", g_cairo_fb.var.bits_per_pixel);
    return -1;
  }

  return 0;
}

int device_close(device_info_t* p_info)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  if (g_cairo_fb.var.bits_per_pixel == 8) {
    set8map(g_cairo_fb.fd, &map_back);
  }

  close(g_cairo_fb.fd);
  free(g_cairo_fb.rgb_buff.c);

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_info->v_ctx;
  scenic_cairo_fini(p_ctx);
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

  cairo_destroy(p_ctx->cr);
  p_ctx->cr = cairo_create(p_ctx->surface);

  // Paint surface to clear color
  cairo_set_source_rgba(p_ctx->cr,
                        p_ctx->clear_color.red,
                        p_ctx->clear_color.green,
                        p_ctx->clear_color.blue,
                        p_ctx->clear_color.alpha);
  cairo_paint(p_ctx->cr);
}

inline static uint8_t to_8_color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((r >> 5) & 7) << 5) |
            (((g >> 5) & 7) << 2) |
            ((b >> 6) & 3));
}

inline static uint16_t to_15_color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((r >> 3) & 31) << 10) |
            (((g >> 3) & 31) << 5)  |
            ((b >> 3) & 31));
}

inline static uint16_t to_15_color_bgr(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((b >> 3) & 31) << 10) |
            (((g >> 3) & 31) << 5)  |
            ((r >> 3) & 31));
}

inline static uint16_t to_16_color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((r >> 3) & 31) << 11) |
            (((g >> 2) & 63) << 5)  |
            ((b >> 3) & 31));
}

void render_cairo_surface_to_fb(scenic_cairo_ctx_t* p_ctx)
{
  cairo_surface_flush(p_ctx->surface);
  uint8_t* cairo_buff = cairo_image_surface_get_data(p_ctx->surface);
  uint32_t width = cairo_image_surface_get_width(p_ctx->surface);
  uint32_t height = cairo_image_surface_get_height(p_ctx->surface);

  bool is_bgr555 = ((g_cairo_fb.var.red.offset == 0 &&
                     g_cairo_fb.var.green.offset == 5 &&
                     g_cairo_fb.var.blue.offset == 10))
                    ? true
                    : false;

  size_t pix_count = width * height;

  int cpp = 0;
  switch (g_cairo_fb.var.bits_per_pixel)
  {
  case 8:
    cpp = 1;
    for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
      g_cairo_fb.rgb_buff.c[i] = to_8_color(cairo_buff[j+2],
                                            cairo_buff[j+1],
                                            cairo_buff[j+0]);
    }
    break;
  case 15:
    cpp = 2;
    if (is_bgr555) {
      for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
        g_cairo_fb.rgb_buff.s[i] = to_15_color_bgr(cairo_buff[j+2],
                                                   cairo_buff[j+1],
                                                   cairo_buff[j+0]);
      }
    } else {
      for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
        g_cairo_fb.rgb_buff.s[i] = to_15_color(cairo_buff[j+2],
                                               cairo_buff[j+1],
                                               cairo_buff[j+0]);
      }
    }
    break;
  case 16:
    cpp = 2;
    if (is_bgr555) {
      for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
        g_cairo_fb.rgb_buff.s[i] = to_15_color_bgr(cairo_buff[j+2],
                                                   cairo_buff[j+1],
                                                   cairo_buff[j+0]);
      }
    } else {
      for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
        g_cairo_fb.rgb_buff.s[i] = to_16_color(cairo_buff[j+2],
                                               cairo_buff[j+1],
                                               cairo_buff[j+0]);
      }
    }
    break;
  case 24:
    cpp = 3;
    for (uint32_t i = 0, j = 0; i < (cpp * pix_count); i += 3, j += 4) {
      g_cairo_fb.rgb_buff.c[i+0] = cairo_buff[j+0];
      g_cairo_fb.rgb_buff.c[i+1] = cairo_buff[j+1];
      g_cairo_fb.rgb_buff.c[i+2] = cairo_buff[j+2];
    }
    break;
  case 32:
    cpp = 4;
    for (uint32_t i = 0, j = 0; i < pix_count; i++, j += 4) {
      g_cairo_fb.rgb_buff.i[i] = ((cairo_buff[j+2] << 16)) |
                                  (cairo_buff[j+1] << 8) |
                                  (cairo_buff[j+0]);
    }
    break;
  }

  uint32_t x_stride = (g_cairo_fb.fix.line_length * 8) / g_cairo_fb.var.bits_per_pixel;

  uint32_t pic_xs = g_device_info.width;
  uint32_t pic_ys = g_device_info.height;
  uint32_t scr_xs = x_stride;
  uint32_t scr_ys = g_cairo_fb.var.yres;

  uint32_t xc = (pic_xs > scr_xs) ? scr_xs : pic_xs;
  uint32_t yc = (pic_ys > scr_ys) ? scr_ys : pic_ys;

  uint32_t x_offs = (pic_xs < g_cairo_fb.var.xres)
                    ? (g_cairo_fb.var.xres - pic_xs) / 2
                    : 0;
  uint32_t y_offs = (pic_ys < g_cairo_fb.var.yres)
                    ? (g_cairo_fb.var.yres - pic_ys) / 2
                    : 0;

  size_t fb_size = scr_xs * scr_ys * cpp;
  uint8_t* fb = mmap(NULL, fb_size, PROT_WRITE | PROT_READ, MAP_SHARED, g_cairo_fb.fd, 0);

  if (fb == MAP_FAILED) {
    log_error("cairo: failed to mmap fb");
    return;
  }

  uint8_t* p_fb = fb + (y_offs * scr_xs + x_offs) * cpp;
  uint8_t* p_image = g_cairo_fb.rgb_buff.c;

  for (uint32_t i = 0; i < yc; i++, p_fb += scr_xs * cpp, p_image += pic_xs * cpp)
    memcpy(p_fb, p_image, xc * cpp);

  munmap(fb, fb_size);
}

void device_end_render(driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;
  render_cairo_surface_to_fb(p_ctx);
}

void device_loop(driver_data_t* p_data)
{
  scenic_loop(p_data);
}

