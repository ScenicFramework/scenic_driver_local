/*
#  Created by Boyd Multerer on 2021-09-16.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "scenic_types.h"
#include "nanovg.h"
#include "font_ops.h"

void init_fonts(void);
void put_font(int* p_msg_length, void* v_ctx);
void set_font(sid_t id, void* v_ctx);
font_t* get_font(sid_t id);

