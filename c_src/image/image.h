/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "scenic_types.h"
#include "tommyhashlin.h"


void init_images( void );
void put_image( int* p_msg_length, NVGcontext* p_ctx );
void reset_images(NVGcontext* p_ctx);

void set_fill_image( NVGcontext* p_ctx, sid_t id );
void set_stroke_image( NVGcontext* p_ctx, sid_t id );


void draw_image(
  NVGcontext* p_ctx, sid_t id,
  float sx, float sy, float sw, float sh,
  float dx, float dy, float dw, float dh
);