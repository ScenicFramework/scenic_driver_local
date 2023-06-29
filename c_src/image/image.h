/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "scenic_types.h"
#include "tommyhashlin.h"

typedef struct {
  sid_t id;
  int32_t image_id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  void* p_pixels;
  tommy_hashlin_node  node;
} image_t;

typedef enum {
  image_format_file = 0,
  image_format_gray = 1,
  image_format_gray_alpha = 2,
  image_format_rgb = 3,
  image_format_rgba = 4,
  image_format_size = 0xffffffff, // ensure enum is 32 bits wide
} image_format_t;

void init_images(void);
void put_image(int* p_msg_length, void* v_ctx);
void reset_images(void* v_ctx);
image_t* get_image(sid_t id);
