#pragma once

#include "scenic_types.h"

int device_ops_init(const device_opts_t* p_opts,
                    device_info_t* p_info,
                    driver_data_t* p_data);
int device_ops_close(device_info_t* p_info);
void device_ops_poll();
void device_ops_begin_render(driver_data_t* p_data);
void device_ops_begin_cursor_render(driver_data_t* p_data);
void device_ops_end_render(driver_data_t* p_data);
void device_ops_clear_color(float red, float green, float blue, float alpha);
char* device_ops_gl_error();
