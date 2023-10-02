#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "cairo_ctx.h"
#include "comms.h"
#include "device.h"
#include "fontstash.h"
#include "script_ops.h"

typedef struct {
  GThread* main;
  GtkWidget* window;
  GMutex render_mutex;
  float last_x;
  float last_y;
} cairo_gtk_t;

cairo_gtk_t g_cairo_gtk;

extern device_info_t g_device_info;
extern device_opts_t g_opts;

static gpointer cairo_gtk_main(gpointer user_data)
{
  gtk_widget_show_all((GtkWidget*)g_cairo_gtk.window);
  gtk_main();

  return NULL;
}

static gboolean on_draw(GtkWidget* widget,
                        cairo_t* cr,
                        gpointer data)
{
  // Don't allow scenic to create a new rendering
  // on p_ctx->surface while gtk is drawing
  g_mutex_lock (&g_cairo_gtk.render_mutex);

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)data;
  cairo_set_source_surface(cr, p_ctx->surface, 0, 0);
  cairo_paint(cr);

  g_mutex_unlock(&g_cairo_gtk.render_mutex);

  return TRUE;
}

static gboolean on_delete_event(GtkWidget* widget,
                                GdkEvent* event,
                                gpointer data)
{
  send_close(0);
  return TRUE;
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
    log_error("cairo %s failed", __func__);
    return -1;
  }

  g_cairo_gtk.last_x = -1.0f;
  g_cairo_gtk.last_y = -1.0f;

  gtk_init(NULL, NULL);

  g_cairo_gtk.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(g_cairo_gtk.window), p_opts->title);
  gtk_window_set_default_size(GTK_WINDOW(g_cairo_gtk.window), p_info->width, p_info->height);
  gtk_window_set_resizable(GTK_WINDOW(g_cairo_gtk.window), FALSE);

  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "delete-event", G_CALLBACK(on_delete_event), NULL);

  GtkDrawingArea* drawing_area = (GtkDrawingArea*)gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(g_cairo_gtk.window), (GtkWidget*)drawing_area);
  g_signal_connect((GtkWidget*)drawing_area, "draw", G_CALLBACK(on_draw), p_ctx);

  g_cairo_gtk.main = g_thread_new("gtk_main", cairo_gtk_main, NULL);

  return 0;
}

int device_close(device_info_t* p_info)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_info->v_ctx;
  gtk_main_quit();
  g_thread_join(g_cairo_gtk.main);
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

  // Don't allow gtk to draw while p_ctx->surface is being rendered
  g_mutex_lock(&g_cairo_gtk.render_mutex);

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

void device_end_render(driver_data_t* p_data)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_data->v_ctx;
  cairo_surface_flush(p_ctx->surface);
  g_mutex_unlock(&g_cairo_gtk.render_mutex);

  g_idle_add((GSourceFunc)gtk_widget_queue_draw, (void*)g_cairo_gtk.window);
}
