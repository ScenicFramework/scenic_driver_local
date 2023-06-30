/*
#  Created by Boyd Multerer on 2021/09/07
#  Copyright 2018-2021 Kry10 Limited
#
*/

#pragma once

#include "scenic_types.h"

int device_init(const device_opts_t* p_opts,
                device_info_t* p_info,
                driver_data_t* p_data);
int device_close(device_info_t* p_info);
void device_poll();
void device_begin_render(driver_data_t* p_data);
void device_begin_cursor_render(driver_data_t* p_data);
void device_end_render(driver_data_t* p_data);
void device_clear_color(float red, float green, float blue, float alpha);
char* device_gl_error();
