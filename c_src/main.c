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

#include "device_ops.h"

device_info_t g_device_info = {0};
device_opts_t g_opts = {0};

//---------------------------------------------------------
int main(int argc, char **argv)
{
  driver_data_t data = {0};

  // super simple arg check
  if ( argc != 10 ) {
    log_error("Wrong number of parameters");
    return -1;
  }

  // ingest the command line options
  g_opts.cursor = atoi(argv[1]);
  g_opts.layer = atoi(argv[2]);
  g_opts.global_opacity = atoi(argv[3]);
  g_opts.antialias = atoi(argv[4]);
  g_opts.debug_mode = atoi(argv[5]);
  g_opts.width = atoi(argv[6]);
  g_opts.height = atoi(argv[7]);
  g_opts.resizable = atoi(argv[8]);
  g_opts.title = argv[9];

  // init the hashtables
  init_scripts();
  init_fonts();
  init_images();

  // prep the driver data
  data.keep_going = true;

  int err = device_ops_init(&g_opts, &g_device_info, &data);
  if (err) {
    log_error("Failed to initilize the device: %d", err);
    return err;
  }

  data.debug_mode = g_opts.debug_mode;
  data.v_ctx = g_device_info.v_ctx;

  // signal the app that the window is ready
  send_ready();

#ifdef SCENIC_GLES2
  log_info("~~~~~~~~~~~~~SCENIC_GLES2 was defined!");
#endif

  /* Loop until the calling app closes the window */
  while (data.keep_going && !isCallerDown()) {
    // check for incoming messages - blocks with a timeout
    handle_stdio_in(&data);
    device_ops_poll();
  }

  device_ops_close(&g_device_info);
  return 0;
}

