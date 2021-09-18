/*
#  Created by Boyd Multerer on 2021/09/07
#  based on old main file
#  Copyright 2018-2021 Kry10 Industries
#
*/

#include <unistd.h>

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <stdint.h>
#include <assert.h>

#include <bcm_host.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "../nanovg/nanovg.h"
#include "../nanovg/nanovg_gl.h"

#include "../types.h"
#include "../comms.h"
#include "device.h"

#define DEFAULT_SCREEN    0

typedef struct {
  EGLDisplay display;
  EGLConfig config;
  EGLSurface surface;
  EGLContext context;
  int major_version;
  int minor_version;
} egl_data_t;

egl_data_t g_egl_data = {0};

//---------------------------------------------------------
// setup the video core
int device_init( device_info_t* p_info ) {

  // initialize the bcm_host from broadcom
  bcm_host_init();

  // query the monitor attached to HDMI
  int width, height;
  if ( graphics_get_display_size( DEFAULT_SCREEN, &width, &height) < 0 ) {
    log_error("RPI driver error: Unable to query the default screen on HDMI");
    return -1;
  }
  p_info->width = width;
  p_info->height = height;
  p_info->ratio = 1.0f;


  //-----------------------------------
  // get an EGL display connection
  EGLBoolean result;

  // get a handle to the display
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if ( display == EGL_NO_DISPLAY ) {
    log_error("RPI driver error: Unable get handle to the default screen on HDMI");
    return -1;
  }
    g_egl_data.display = display;


  // initialize the EGL display connection
  EGLint major_version;
  EGLint minor_version;
  // returns a pass/fail boolean
  if ( eglInitialize(display, &major_version, &minor_version) == EGL_FALSE ) {
    log_error("RPI driver error: Unable initialize EGL");
    return -1;
  }
  g_egl_data.major_version = major_version;
  g_egl_data.minor_version = minor_version;


  // prepare an appropriate EGL frame buffer configuration request
  static const EGLint attribute_list[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_STENCIL_SIZE, 1,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE
  };
  static const EGLint context_attributes[] = {
     EGL_CONTEXT_CLIENT_VERSION, 2,
     EGL_NONE
   };
  EGLConfig config;
  EGLint num_config;


   // get an appropriate EGL frame buffer configuration
  if ( eglChooseConfig(display, attribute_list, &config, 1, &num_config) == EGL_FALSE ) {
    log_error("RPI driver error: Unable to get usable display config");
    return -1;
  }
  g_egl_data.config = config;


  // use open gl es
  if ( eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE ) {
    log_error("RPI driver error: Unable to bind to GLES");
    return -1;
  }


  // create an EGL graphics context
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
  if ( context == EGL_NO_CONTEXT ) {
    log_error("RPI driver error: Failed to create EGL context");
    return -1;
  }
  g_egl_data.context = context;

  //-------------------
  // create the native window and bind it

  static EGL_DISPMANX_WINDOW_T nativewindow;
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  dst_rect.x = 0;
  dst_rect.y = 0;
  if ( p_info->debug_mode ) {
    dst_rect.width = width / 2;
    dst_rect.height = height / 2;
  } else {
    dst_rect.width = width;
    dst_rect.height = height;
  }

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = width << 16;
  src_rect.height = height << 16;

  // start the display manager
  DISPMANX_DISPLAY_HANDLE_T dispman_display = vc_dispmanx_display_open(0 /* LCD */);
  dispman_update = vc_dispmanx_update_start(0 /* LCD */);


  // create the screen element (will be full-screen)
  VC_DISPMANX_ALPHA_T alpha =
  {
      DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
      p_info->global_opacity, /*alpha 0->255*/
      0
  };
  DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
    dispman_update,
    dispman_display,
    p_info->layer,
    &dst_rect,
    0/*src*/,
    &src_rect,
    DISPMANX_PROTECTION_NONE,
    &alpha, 0/*clamp*/, 0/*transform*/
  );
  result = vc_dispmanx_update_submit_sync(dispman_update);
  if (result != 0) {
    log_error("RPI driver error: Unable to start dispmanx element");
    return -1;
  }


  // create the native window surface
  nativewindow.element = dispman_element;
  nativewindow.width = width;
  nativewindow.height = height;
  EGLSurface surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
  if (surface == EGL_NO_SURFACE) {
    log_error("RPI driver error: Unable create the native window surface");
    return -1;
  }
  g_egl_data.surface = surface;

  // connect the context to the surface and make it current
  if ( eglMakeCurrent(display, surface, surface, context) == EGL_FALSE ) {
    log_error("RPI driver error: Unable make the surface current");
    return -1;
  }


  //-------------------
  // config gles

  // set the view port to the new size passed in
  glViewport(0, 0, width, height);

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


  //-------------------
  // initialize nanovg

  // p_info->p_ctx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  p_info->p_ctx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_DEBUG);
  if (p_info->p_ctx == NULL) {
    log_error("RPI driver error: failed nvgCreateGLES2");
    return 0;
  }

  // tell the elixir side about the size/shape of the window
  send_reshape( width, height );

  // success
  return 0;
}

int device_close( device_info_t* p_info ) {
  return 0;
}

int device_swap_buffers() {
  eglSwapBuffers( g_egl_data.display, g_egl_data.surface );
  return 0;
}

void device_poll() {}
