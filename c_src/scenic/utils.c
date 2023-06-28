/*
#  Created by Boyd Multerer on 2/25/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#
*/

#include "common.h"
#include "device_ops.h"

void check_gl_error() {
  char* p_err_str;
  while((p_err_str = device_ops_gl_error()) != NULL) {
    log_error("%s", p_err_str);
  }
}
