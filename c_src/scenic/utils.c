/*
#  Created by Boyd Multerer on 2/25/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#

Functions to load fonts and render text
*/


#include "common.h"
#include "device.h"

// #include <stdio.h>
// #include <string.h>

void check_gl_error() {
  char* p_err_str;
  while( p_err_str = device_gl_error() ) {
    send_puts( p_err_str );
  }
}

