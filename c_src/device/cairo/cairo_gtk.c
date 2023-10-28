#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
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
#include "scenic_ops.h"
#include "script_ops.h"

typedef struct {
  GThread* main;
  GtkWidget* window;
  GMutex render_mutex;
  GMutex cmd_mutex;
  float last_x;
  float last_y;
} cairo_gtk_t;

cairo_gtk_t g_cairo_gtk;

extern device_info_t g_device_info;
extern device_opts_t g_opts;

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

static gboolean on_motion_event(GtkWidget* widget,
                                GdkEventMotion* event,
                                gpointer data)
{
  float x = floorf(event->x);
  float y = floorf(event->y);

  if ((g_cairo_gtk.last_x != x) && (g_cairo_gtk.last_y != y)) {
    send_cursor_pos(x, y);
    g_cairo_gtk.last_x = x;
    g_cairo_gtk.last_y = y;
  }

  return TRUE;
}

static gboolean on_button_event(GtkWidget* widget,
                                GdkEventButton* event,
                                gpointer data)
{
  int action;
  switch (event->type) {
  case GDK_BUTTON_PRESS:
    action = 1;
    break;
  case GDK_BUTTON_RELEASE:
    action = 0;
    break;
  default:
    return FALSE;
  }

  float x = floorf(event->x);
  float y = floorf(event->y);

  send_mouse_button(KEYMAP_GDK,
                    event->button,
                    action,
                    event->state,
                    x, y);

  return TRUE;
}

static gboolean on_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer data)
{
  int action = (event->type == GDK_KEY_PRESS) ? 1 : 0;
  uint32_t unicode = gdk_keyval_to_unicode(event->keyval);
  send_key(KEYMAP_GDK, event->keyval, event->hardware_keycode, action, event->state);
  if (!(event->keyval & 0xF000) && event->type == GDK_KEY_PRESS) {
    send_codepoint(KEYMAP_GDK, unicode, event->state);
  }
  return TRUE;
}

static gboolean on_enter_leave_event(GtkWidget* widget,
                                     GdkEventCrossing* event,
                                     gpointer data)
{
  int action = (event->type == GDK_ENTER_NOTIFY) ? 1 : 0;
  float x = floorf(event->x);
  float y = floorf(event->y);

  send_cursor_enter(action, x, y);

  return TRUE;
}

static gboolean on_scroll_event(GtkWidget* widget,
                                GdkEventScroll* event,
                                gpointer data)
{
  float x = floorf(event->x);
  float y = floorf(event->y);
  float xoffset = 0.0;
  float yoffset = 0.0;

  switch (event->direction) {
  case GDK_SCROLL_UP:    yoffset = -1.0; break;
  case GDK_SCROLL_DOWN:  yoffset = 1.0; break;
  case GDK_SCROLL_LEFT:  xoffset = 1.0; break;
  case GDK_SCROLL_RIGHT: xoffset = -1.0; break;
  case GDK_SCROLL_SMOOTH: return FALSE;
  }
  send_scroll(xoffset, yoffset, x, y);

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

  gtk_widget_set_events(g_cairo_gtk.window,
                        GDK_POINTER_MOTION_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_SCROLL_MASK );

  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "motion-notify-event", G_CALLBACK(on_motion_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "button-press-event", G_CALLBACK(on_button_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "button-release-event", G_CALLBACK(on_button_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "key-press-event", G_CALLBACK(on_key_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "key-release-event", G_CALLBACK(on_key_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "enter-notify-event", G_CALLBACK(on_enter_leave_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "leave-notify-event", G_CALLBACK(on_enter_leave_event), NULL);
  g_signal_connect(G_OBJECT(g_cairo_gtk.window), "scroll-event", G_CALLBACK(on_scroll_event), NULL);

  GtkDrawingArea* drawing_area = (GtkDrawingArea*)gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(g_cairo_gtk.window), (GtkWidget*)drawing_area);
  g_signal_connect((GtkWidget*)drawing_area, "draw", G_CALLBACK(on_draw), p_ctx);

  return 0;
}

int device_close(device_info_t* p_info)
{
  if (g_opts.debug_mode) {
    log_info("cairo %s", __func__);
  }

  scenic_cairo_ctx_t* p_ctx = (scenic_cairo_ctx_t*)p_info->v_ctx;
  gtk_main_quit();
  scenic_cairo_fini(p_ctx);

  return 0;
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

void glib_print(const gchar* string)
{
  log_info("glib: %s", string);
}

void glib_error(const gchar* string)
{
  log_error("glib: %s", string);
}

void scenic_cmd_lock()
{
  g_mutex_lock(&g_cairo_gtk.cmd_mutex);
}

void scenic_cmd_unlock()
{
  g_mutex_unlock(&g_cairo_gtk.cmd_mutex);
}

void device_loop(driver_data_t* p_data)
{
  g_cairo_gtk.main = g_thread_new("scenic_loop", scenic_loop, p_data);

  g_set_print_handler(glib_print);
  g_set_printerr_handler(glib_error);

  gtk_widget_show_all((GtkWidget*)g_cairo_gtk.window);
  gtk_main();
}
