#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

#ifdef SCENIC_GLES2
  #include <GLES3/gl2.h>
  #define NANOVG_GLES2_IMPLEMENTATION
#else
  #include <GLES3/gl3.h>
  #define NANOVG_GLES3_IMPLEMENTATION
#endif
#include <EGL/egl.h>
#include "nanovg.h"
#include "nanovg_gl.h"

#include "scenic_types.h"
#include "comms.h"
#include "device.h"

#define DEFAULT_SCREEN    0

#define STDIN_FILENO 0
#define DEFAULT_SCREEN 0
#define MSG_OUT_PUTS 0x02
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MAX_DISPLAYS  (4)
#define MAX_BUFFERS   (4)


static void page_flip_handler(int fd, unsigned int frame,
      unsigned int sec, unsigned int usec, void *data)
{
  int *waiting_for_flip = data;
  *waiting_for_flip = *waiting_for_flip - 1;
}


uint8_t DISP_ID = 0;
uint8_t all_display = 0;
int8_t connector_id = -1;
char* device = "/dev/dri/card0";

fd_set fds;
drmEventContext evctx = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = page_flip_handler,
};


typedef struct {
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  int screen_width;
  int screen_height;
  int frame_idx;
  int major_version;
  int minor_version;
} egl_data_t;

egl_data_t g_egl_data = {0};

static struct {
  struct gbm_device *dev;
  struct gbm_surface *surface;
  struct gbm_bo *bo[MAX_BUFFERS];
} gbm;

static struct {
  int fd;
  uint32_t ndisp;
  uint32_t crtc_id[MAX_DISPLAYS];
  uint32_t connector_id[MAX_DISPLAYS];
  drmModeEncoder* encoder[MAX_DISPLAYS];
  uint32_t format[MAX_DISPLAYS];
  drmModeModeInfo *mode[MAX_DISPLAYS];
  drmModeConnector *connectors[MAX_DISPLAYS];
  struct drm_fb *fb[MAX_BUFFERS];
} drm;

struct drm_fb {
  struct gbm_bo *bo;
  uint32_t fb_id;
};

static uint32_t drm_fmt_to_gbm_fmt(uint32_t fmt)
{
  switch (fmt) {
    case DRM_FORMAT_XRGB8888:
      return GBM_FORMAT_XRGB8888;
    case DRM_FORMAT_ARGB8888:
      return GBM_FORMAT_ARGB8888;
    case DRM_FORMAT_RGB565:
      return GBM_FORMAT_RGB565;
    default:
      fprintf(stderr, "Unsupported DRM format: 0x%x", fmt);
      return GBM_FORMAT_XRGB8888;
  }
}

static void
drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
  struct drm_fb *fb = data;
  struct gbm_device *gbm = gbm_bo_get_device(bo);

  if (fb->fb_id)
    drmModeRmFB(drm.fd, fb->fb_id);

  free(fb);
}

static struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
  struct drm_fb *fb = gbm_bo_get_user_data(bo);
  uint32_t width, height, format;
  uint32_t bo_handles[4] = {0}, offsets[4] = {0}, pitches[4] = {0};
  int ret;

  if (fb)
    return fb;

  fb = calloc(1, sizeof *fb);
  fb->bo = bo;

  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);
  pitches[0] = gbm_bo_get_stride(bo);
  bo_handles[0] = gbm_bo_get_handle(bo).u32;
  format = gbm_bo_get_format(bo);

  ret = drmModeAddFB2(drm.fd, width, height, format, bo_handles, pitches, offsets, &fb->fb_id, 0);
  if (ret) {
    printf("failed to create fb: %s\n", strerror(errno));
    free(fb);
    return NULL;
  }

  gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

  return fb;
}

static bool search_plane_format(uint32_t desired_format, int formats_count, uint32_t* formats)
{
  int i;

  for ( i = 0; i < formats_count; i++)
  {
    if (desired_format == formats[i])
      return true;
  }

  return false;
}

int get_drm_prop_val(int fd, drmModeObjectPropertiesPtr props,
                     const char *name, unsigned int *p_val)
{
  drmModePropertyPtr p;
  unsigned int i, prop_id = 0; /* Property ID should always be > 0 */

  for (i = 0; !prop_id && i < props->count_props; i++) {
    p = drmModeGetProperty(fd, props->props[i]);
    if (!strcmp(p->name, name)) {
      prop_id = p->prop_id;
      break;
    }
    drmModeFreeProperty(p);
  }

  if (!prop_id) {
    fprintf(stderr, "Could not find %s property\n", name);
    return(-1);
  }

  drmModeFreeProperty(p);
  *p_val = props->prop_values[i];
  return 0;
}


static bool set_drm_format(void)
{
  /* desired DRM format in order */
  static const uint32_t drm_formats[] = {DRM_FORMAT_XRGB8888, DRM_FORMAT_ARGB8888, DRM_FORMAT_RGB565};
  drmModePlaneRes *plane_res;
  bool found = false;
  int i,k;

  drmSetClientCap(drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  plane_res  = drmModeGetPlaneResources(drm.fd);

  if (!plane_res) {
    fprintf(stderr, "drmModeGetPlaneResources failed: %s\n", strerror(errno));
    drmSetClientCap(drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
    return false;
  }

  /*
   * find the plane connected to crtc_id (the primary plane) and then find the desired pixel format
   * from the plane format list
   */
  for (i = 0; i < plane_res->count_planes; i++)
  {
    drmModePlane *plane = drmModeGetPlane(drm.fd, plane_res->planes[i]);
    drmModeObjectProperties *props;
    unsigned int plane_type;

    if(plane == NULL)
      continue;

    props = drmModeObjectGetProperties(drm.fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);

    if(props == NULL){
      fprintf(stderr, "plane (%d) properties not found\n",  plane->plane_id);
      drmModeFreePlane(plane);
      continue;
    }

    if(get_drm_prop_val(drm.fd, props, "type",  &plane_type) < 0)
    {
      fprintf(stderr, "plane (%d) type value not found\n",  plane->plane_id);
      drmModeFreeObjectProperties(props);
      drmModeFreePlane(plane);
      continue;
    }

    if (plane_type != DRM_PLANE_TYPE_PRIMARY)
    {
      drmModeFreeObjectProperties(props);
      drmModeFreePlane(plane);
      continue;
    }
    else if (!plane->crtc_id)
    {
      plane->crtc_id = drm.crtc_id[drm.ndisp];
    }

    drmModeFreeObjectProperties(props);

    if (plane->crtc_id == drm.crtc_id[drm.ndisp])
    {
      for (k = 0; k < ARRAY_SIZE(drm_formats); k++)
      {
        if (search_plane_format(drm_formats[k], plane->count_formats, plane->formats))
        {
          drm.format[drm.ndisp] = drm_formats[k];
          drmModeFreePlane(plane);
          drmModeFreePlaneResources(plane_res);
          drmSetClientCap(drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
          return true;
        }
      }
    }

    drmModeFreePlane(plane);
  }

  drmModeFreePlaneResources(plane_res);
  drmSetClientCap(drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
  return false;
}


static int init_drm(const device_opts_t* p_opts, device_info_t* p_info)
{
  drmModeRes *resources;
  drmModeConnector *connector = NULL;
  drmModeEncoder *encoder = NULL;
  drmModeCrtc *crtc = NULL;

  int i, j, k;
  uint32_t maxRes, curRes;

  /* Open default dri device */
  drm.fd = open(device, O_RDWR | O_CLOEXEC);
  if (drm.fd < 0) {
    fprintf(stderr, "could not open drm device %s\n", device);
    return -1;
  }

  resources = drmModeGetResources(drm.fd);
  if (!resources) {
    fprintf(stderr, "drmModeGetResources failed: %s\n", strerror(errno));
    return -1;
  }
  // drm.resource_id = (uint32_t) resources;


  /* find a connected connector: */
  for (i = 0; i < resources->count_connectors; i++) {
    connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
    if (connector->connection == DRM_MODE_CONNECTED) {

      /* find the matched encoders */
      for (j=0; j<connector->count_encoders; j++) {
        encoder = drmModeGetEncoder(drm.fd, connector->encoders[j]);

        /* Take the first one, if none is assigned */
        if (!connector->encoder_id)
        {
          connector->encoder_id = encoder->encoder_id;
        }

        if (encoder->encoder_id == connector->encoder_id)
        {
          /* find the first valid CRTC if not assigned */
          if (!encoder->crtc_id)
          {
            for (k = 0; k < resources->count_crtcs; ++k) {
              /* check whether this CRTC works with the encoder */
              if (!(encoder->possible_crtcs & (1 << k)))
                continue;

              encoder->crtc_id = resources->crtcs[k];
              break;
            }

            if (!encoder->crtc_id)
            {
              fprintf(stderr, "Encoder(%d): no CRTC find!\n", encoder->encoder_id);
              drmModeFreeEncoder(encoder);
              encoder = NULL;
              continue;
            }
          }

          break;
        }

        drmModeFreeEncoder(encoder);
        encoder = NULL;
      }

      if (!encoder) {
        fprintf(stderr, "Connector (%d): no encoder!\n", connector->connector_id);
        drmModeFreeConnector(connector);
        continue;
      }

      /* choose the current or first supported mode */
      crtc = drmModeGetCrtc(drm.fd, encoder->crtc_id);
      for (j = 0; j < connector->count_modes; j++)
      {
        if (crtc->mode_valid)
        {
          if ((connector->modes[j].hdisplay == crtc->width) &&
          (connector->modes[j].vdisplay == crtc->height))
          {
            drm.mode[drm.ndisp] = &connector->modes[j];
            break;
          }
        }
        else
        {
          if ((connector->modes[j].hdisplay == crtc->x) &&
             (connector->modes[j].vdisplay == crtc->y))
          {
            drm.mode[drm.ndisp] = &connector->modes[j];
            break;
          }
        }
      }

      if(j >= connector->count_modes)
        drm.mode[drm.ndisp] = &connector->modes[0];

      drm.connector_id[drm.ndisp] = connector->connector_id;
      drm.encoder[drm.ndisp]  = encoder;
      drm.crtc_id[drm.ndisp] = encoder->crtc_id;
      drm.connectors[drm.ndisp] = connector;


      if (!set_drm_format())
      {
        // Error handling
        fprintf(stderr, "No desired pixel format found!\n");
        return -1;
      }

      fprintf(stderr, "### Display [%d]: CRTC = %d, Connector = %d, format = 0x%x\n", drm.ndisp, drm.crtc_id[drm.ndisp], drm.connector_id[drm.ndisp], drm.format[drm.ndisp]);
      fprintf(stderr, "\tMode chosen [%s] : Clock => %d, Vertical refresh => %d, Type => %d\n", drm.mode[drm.ndisp]->name, drm.mode[drm.ndisp]->clock, drm.mode[drm.ndisp]->vrefresh, drm.mode[drm.ndisp]->type);
      fprintf(stderr, "\tHorizontal => %d, %d, %d, %d, %d\n", drm.mode[drm.ndisp]->hdisplay, drm.mode[drm.ndisp]->hsync_start, drm.mode[drm.ndisp]->hsync_end, drm.mode[drm.ndisp]->htotal, drm.mode[drm.ndisp]->hskew);
      fprintf(stderr, "\tVertical => %d, %d, %d, %d, %d\n", drm.mode[drm.ndisp]->vdisplay, drm.mode[drm.ndisp]->vsync_start, drm.mode[drm.ndisp]->vsync_end, drm.mode[drm.ndisp]->vtotal, drm.mode[drm.ndisp]->vscan);
      g_egl_data.screen_width = drm.mode[drm.ndisp]->hdisplay;
      g_egl_data.screen_height = drm.mode[drm.ndisp]->vdisplay;
      p_info->width = drm.mode[drm.ndisp]->hdisplay;
      p_info->height = drm.mode[drm.ndisp]->vdisplay;
      p_info->ratio = 1.0f;


      /* If a connector_id is specified, use the corresponding display */
      if ((connector_id != -1) && (connector_id == drm.connector_id[drm.ndisp]))
        DISP_ID = drm.ndisp;

      /* If all displays are enabled, choose the connector with maximum
      * resolution as the primary display */
      if (all_display) {
        maxRes = drm.mode[DISP_ID]->vdisplay * drm.mode[DISP_ID]->hdisplay;
        curRes = drm.mode[drm.ndisp]->vdisplay * drm.mode[drm.ndisp]->hdisplay;

        if (curRes > maxRes)
          DISP_ID = drm.ndisp;
      }

      drm.ndisp++;
    } else {
      drmModeFreeConnector(connector);
    }
  }

  if (drm.ndisp == 0) {
    /* we could be fancy and listen for hotplug events and wait for
     * a connector..
     */
    fprintf(stderr, "no connected connector!\n");
    return -1;
  }

  return 0;
}

static int init_gbm(void)
{
  gbm.dev = gbm_create_device(drm.fd);

  gbm.surface = gbm_surface_create(gbm.dev,
                                   drm.mode[DISP_ID]->hdisplay,
                                   drm.mode[DISP_ID]->vdisplay,
                                   drm_fmt_to_gbm_fmt(drm.format[DISP_ID]),
                                   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  if (!gbm.surface) {
    fprintf(stderr, "failed to create gbm surface\n");
    return -1;
  }

  return 0;
}




int init_egl(const device_opts_t* p_opts, device_info_t* p_info)
{
  EGLint major, minor, n;
  GLint ret;

  static const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  static const EGLint config_attribs[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_STENCIL_SIZE, 1,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE
  };

  g_egl_data.display = eglGetDisplay(gbm.dev);
  if (!eglInitialize(g_egl_data.display, &g_egl_data.major_version, &g_egl_data.minor_version)) {
    fprintf(stderr, "failed to initialize egl\n");
    return -1;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    fprintf(stderr, "failed to bind api EGL_OPENGL_ES_API\n");
    return -1;
  }

  EGLConfig config = NULL;
  if ( eglChooseConfig(g_egl_data.display, config_attribs, &config, 1, &n) != EGL_TRUE ) {
    fprintf( stderr, "failed to choose config\n" );
    return -1;
  }

  g_egl_data.context = eglCreateContext(g_egl_data.display, config,
      EGL_NO_CONTEXT, context_attribs);
  if (g_egl_data.context == NULL) {
    fprintf(stderr, "failed to create context\n");
    return -1;
  }

  g_egl_data.surface = eglCreateWindowSurface(g_egl_data.display, config, gbm.surface, NULL);
  if (g_egl_data.surface == EGL_NO_SURFACE) {
    fprintf(stderr, "failed to create egl surface\n");
    return -1;
  }

  /* connect the context to the surface */
  eglMakeCurrent(g_egl_data.display, g_egl_data.surface, g_egl_data.surface, g_egl_data.context);

  fprintf(stderr, "connected surface\n");
  //-------------------
  // config gles

  // set the view port to the new size passed in
  glViewport(0, 0, p_info->width, p_info->height);

  // This turns on/off depth test.
  // With this ON, whatever we draw FIRST is
  // "on top" and each subsequent draw is BELOW
  // the draw calls before it.
  // With this OFF, whatever we draw LAST is
  // "on top" and each subsequent draw is ABOVE
  // the draw calls before it.
  glDisable(GL_DEPTH_TEST);

  // Probably need this on, enables Gouraud Shading
  // glShadeModel(GL_SMOOTH);

  // Turn on Alpha Blending
  // There are some efficiencies to be gained by ONLY
  // turning this on when we have a primitive with a
  // style that has an alpha channel != 1.0f but we
  // don't have code to detect that.  Easy to do if we need it!
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  fprintf(stderr, "configured gles\n");

  uint32_t nvg_opts = 0;
  if (p_opts->antialias) nvg_opts |= NVG_ANTIALIAS;
  if (p_opts->debug_mode) nvg_opts |= NVG_DEBUG;

#ifdef SCENIC_GLES2
  p_info->p_ctx = nvgCreateGLES2(nvg_opts);
#else
  p_info->p_ctx = nvgCreateGLES3(nvg_opts);
#endif

  if (p_info->p_ctx == NULL)
  {
    fprintf(stderr, "Failed to create nvg\n");
    send_puts("EGL driver error: failed nvgCreateGLES2");
    return -1;
  }

  return 0;
}



int device_init(const device_opts_t* p_opts,
                device_info_t* p_info)
{
  int ret;

  // initialize
  ret = init_drm(p_opts, p_info);
  if (ret) {
    fprintf(stderr, "failed to initialize DRM\n");
    return ret;
  }

  fprintf(stderr, "### Primary display => ConnectorId = %d, Resolution = %dx%d\n",
    drm.connector_id[DISP_ID], drm.mode[DISP_ID]->hdisplay,
    drm.mode[DISP_ID]->vdisplay);

  FD_ZERO(&fds);
  FD_SET(drm.fd, &fds);

  ret = init_gbm();
  if (ret) {
    fprintf(stderr, "failed to initialize GBM\n");
    return ret;
  }

  ret = init_egl( p_opts, p_info );
  if (ret) {
    fprintf(stderr, "failed to initialize EGL\n");
    return ret;
  }


  g_egl_data.frame_idx = 0;

  glClearColor(0.5f, 0.1f, 0.7f, 1.0f);

  eglSwapBuffers(g_egl_data.display, g_egl_data.surface);
  struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm.surface);

  gbm.bo[g_egl_data.frame_idx] = bo;

  drm.fb[g_egl_data.frame_idx] = drm_fb_get_from_bo(bo);
  ret = drmModeSetCrtc(drm.fd, drm.crtc_id[DISP_ID], drm.fb[g_egl_data.frame_idx]->fb_id,
                       0, 0, &drm.connector_id[DISP_ID], 1, drm.mode[DISP_ID]);
  if (ret) {
    printf("display %d failed to set mode: %s", DISP_ID, strerror(errno));
    return ret;
  }

  return 0;
}

int device_close(device_info_t* p_info)
{
  return 0;
}

void device_begin_render()
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void device_end_render()
{
  int waiting_for_flip;
  int cc, ret;
  int next_idx;
  if (g_egl_data.frame_idx == (MAX_BUFFERS - 1)) {
    next_idx = 0;
  } else {
    next_idx = g_egl_data.frame_idx + 1;
  }

  eglSwapBuffers(g_egl_data.display, g_egl_data.surface);

  gbm.bo[next_idx] = gbm_surface_lock_front_buffer(gbm.surface);
  drm.fb[next_idx] = drm_fb_get_from_bo(gbm.bo[next_idx]);
  ret = drmModeSetCrtc(drm.fd, drm.crtc_id[DISP_ID], drm.fb[next_idx]->fb_id,
                       0, 0, &drm.connector_id[DISP_ID], 1, drm.mode[DISP_ID]);
  if (ret) {
    log_error("device_swap_buffers display failed to set mode");
    printf("display %d failed to set mode: %s\n", DISP_ID, strerror(errno));
    return;
  }

  ret = drmModePageFlip(drm.fd, drm.crtc_id[DISP_ID], drm.fb[next_idx]->fb_id,
                        DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
  if (ret) {
    log_error("failed to queue page flip");
    fprintf(stderr, "failed to queue page flip: %s\n", strerror(errno));
    return;
  }

  
  waiting_for_flip = 1;
  while (waiting_for_flip) {
    ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
    if (ret < 0) {
      log_error("select err");
      fprintf(stderr, "select err: %s\n", strerror(errno));
      return;
    } else if (ret == 0) {
      log_error("select timeout!");
      fprintf(stderr, "select timeout!\n");
      return;
    } else if (FD_ISSET(0, &fds)) {
      continue;
    }
    drmHandleEvent(drm.fd, &evctx);
  }

  if (gbm.bo[g_egl_data.frame_idx]) {
    gbm_surface_release_buffer(gbm.surface, gbm.bo[g_egl_data.frame_idx]);
  }
  g_egl_data.frame_idx = next_idx;
}

void device_poll()
{
}

// these case are factored out mostly so that they can be driven by different
// GL includes as appropriate
void device_clear()
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void device_clear_color(float red,
                        float green,
                        float blue,
                        float alpha)
{
  glClearColor(red, green, blue, alpha);
}

char* device_gl_error()
{
  GLenum err  = glGetError();
  switch (err)
  {
  case GL_NO_ERROR: return NULL;
  case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
  case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#ifdef GL_STACK_UNDERFLOW
  case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
#endif
#ifdef GL_STACK_OVERFLOW
  case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
#endif
  case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
  default: return "GL_OTHER";
  }
}
