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

#include "comms.h"
#include "scenic_types.h"
#include "image.h"
#include "font.h"
#include "script.h"

#include "device.h"

#include "nanovg.h"
#include "nanovg_gl.h"

device_info_t g_device_info = {0};

//---------------------------------------------------------
int main(int argc, char **argv)
{
  driver_data_t data = {0};

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

  // init the hashtables
  init_scripts();
  init_fonts();
  init_images();

  // initialize the global transform to the identity matrix
  nvgTransformIdentity( data.global_tx );
  nvgTransformIdentity( data.cursor_tx );

  // prep the driver data
  data.keep_going = true;

  int err = device_init(&opts, &g_device_info);
  if (err) {
    send_puts( "Failed to initilize the device" );
    return err;
  }

  data.p_ctx = g_device_info.p_ctx;

  // signal the app that the window is ready
  send_ready();

#ifdef SCENIC_GLES2
  send_puts("~~~~~~~~~~~~~SCENIC_GLES2 was defined!");
#endif

  /* Loop until the calling app closes the window */
  while (data.keep_going && !isCallerDown()) {
    // check for incoming messages - blocks with a timeout
    handle_stdio_in(&data);
    device_poll();
  }

  device_close(&g_device_info);
  return 0;
}

