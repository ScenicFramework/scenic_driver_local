/*
#  Created by Boyd Multerer on 2021/09/07
#  Copyright 2018-2021 Kry10 Limited
#
*/

#pragma once

int device_init(const device_opts_t* p_opts,
                device_info_t* p_info);
int device_close(device_info_t* p_info);
void device_poll();
void device_begin_render();
void device_end_render();
void device_clear_color(float red, float green, float blue, float alpha);
char* device_gl_error();
