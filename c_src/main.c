/*
#  Created by Boyd Multerer on 05/17/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
*/

#include <unistd.h>

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <stdint.h>
#include <assert.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// #include <EGL/egl.h>
// #include <EGL/eglext.h>

#include "comms.h"
#include "types.h"
#include "image.h"
#include "font.h"
#include "script.h"

#include "device/device.h"

#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"


#define STDIN_FILENO 0
#define DEFAULT_SCREEN    0


device_info_t g_device_info = {0};


void test_draw(NVGcontext* p_ctx) {
  //-----------------------------------
  // Set background color and clear buffers
  // glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
  // glClearColor(0.098f, 0.098f, 0.439f, 1.0f);    // midnight blue
  // glClearColor(0.545f, 0.000f, 0.000f, 1.0f);    // dark red
  glClearColor(0.184f, 0.310f, 0.310f, 1.0f);       // dark slate gray
  // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);       // black

  glClear(GL_COLOR_BUFFER_BIT);

  int width = g_device_info.width;
  int height = g_device_info.height;
  int ratio = g_device_info.ratio;

  nvgBeginFrame(p_ctx, width, height, ratio);

    // Next, draw graph line
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, 0, 0);
  nvgLineTo(p_ctx, width, height);
  nvgStrokeColor(p_ctx, nvgRGBA(0, 160, 192, 255));
  nvgStrokeWidth(p_ctx, 3.0f);
  nvgStroke(p_ctx);

  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, width, 0);
  nvgLineTo(p_ctx, 0, height);
  nvgStrokeColor(p_ctx, nvgRGBA(0, 160, 192, 255));
  nvgStrokeWidth(p_ctx, 3.0f);
  nvgStroke(p_ctx);

  nvgBeginPath(p_ctx);
  nvgCircle(p_ctx, width / 2, height / 2, 50);
  nvgFillColor(p_ctx, nvgRGBAf(0.545f, 0.000f, 0.000f, 1.0f));
  nvgFill(p_ctx);
  nvgStroke(p_ctx);

  nvgEndFrame(p_ctx);
  device_swap_buffers();
}


//---------------------------------------------------------
int main(int argc, char **argv) {
  driver_data_t     data = {0};

  // test_endian();

log_info("start main");

  // super simple arg check
  if ( argc != 10 ) {
    log_error("Wrong number of parameters");
    return 0;
  }

  // ingest the command line options
  device_opts_t opts = {0};
  opts.cursor = atoi(argv[1]);
  opts.layer = atoi(argv[2]);
  opts.global_opacity = atoi(argv[3]);
  opts.antialias = atoi(argv[4]);  
  opts.debug_mode = atoi(argv[5]);
  opts.width = atoi(argv[6]);
  opts.height = atoi(argv[7]);
  opts.resizable = atoi(argv[8]);
  opts.title = argv[9];

log_info("init the hashtables");

  // init the hashtables
  init_scripts();
  init_fonts();
  init_images();


log_info("init transforms");

  // initialize the global transform to the identity matrix
  nvgTransformIdentity( data.global_tx );
  nvgTransformIdentity( data.cursor_tx );


  // prep the driver data
  data.keep_going = true;

log_info("device_init");

  int err = device_init( &opts, &g_device_info );
  if ( err ) {
    send_puts( "Failed to initilize the device" );
    return err;
  }
  data.p_ctx = g_device_info.p_ctx;

log_info("send_ready");
  // signal the app that the window is ready
  send_ready();

log_info("start loop");
  /* Loop until the calling app closes the window */
  while ( data.keep_going && !isCallerDown() ) {
    // check for incoming messages - blocks with a timeout
    handle_stdio_in( &data );
    device_poll();
  }

  device_close( &g_device_info );
  return 0;
}

