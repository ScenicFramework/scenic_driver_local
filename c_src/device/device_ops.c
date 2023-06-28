#include "device_ops.h"
#include <stddef.h>

__attribute__((weak))
int device_ops_init(const device_opts_t* p_opts,
                    device_info_t* p_info,
                    driver_data_t* p_data)
{
  return 0;
}

__attribute__((weak))
int device_ops_close(device_info_t* p_info)
{
  return 0;
}

__attribute__((weak))
void device_ops_poll()
{
}

__attribute__((weak))
void device_ops_begin_render(driver_data_t* p_data)
{
}

__attribute__((weak))
void device_ops_begin_cursor_render(driver_data_t* p_data)
{
}

__attribute__((weak))
void device_ops_end_render(driver_data_t* p_data)
{
}

__attribute__((weak))
void device_ops_clear_color(float red, float green, float blue, float alpha)
{
}

__attribute__((weak))
char* device_ops_gl_error()
{
  return NULL;
}
