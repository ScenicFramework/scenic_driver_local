#pragma once

#include "scenic_types.h"

typedef enum {
  scenic_op_put_script = 0x01,
  scenic_op_del_script = 0x02,
  scenic_op_reset = 0x03,
  scenic_op_global_tx = 0x04,
  scenic_op_cursor_tx = 0x05,
  scenic_op_render = 0x06,
  scenic_op_update_cursor = 0x07,
  scenic_op_clear_color = 0x08,

  //scenic_op_input = 0x0a,

  scenic_op_quit = 0x20,

  scenic_op_put_font = 0x40,
  scenic_op_put_image = 0x41,

  // scenic_op_query_stats = 0x21,
  // scenic_op_reshap = 0x22,
  // scenic_op_position = 0x23,
  // scenic_op_focus = 0x24,
  // scenic_op_iconify = 0x25,
  // scenic_op_maximize = 0x26,
  // scenic_op_restore = 0x27,
  // scenic_op_show = 0x28,
  // scenic_op_hide = 0x29,

  // scenic_op_new_tx_id = 0x32,
  // scenic_op_free_tx_id = 0x33,
  // scenic_op_put_tx_blob = 0x34,
  // scenic_op_put_tx_raw = 0x35,

  // scenic_op_load_font_file = 0x37,
  // scenic_op_load_font_blob = 0x38,
  // scenic_op_free_font = 0x39.

  scenic_op_crash = 0xfe,
} scenic_op_t;

void scenic_ops_put_script(uint32_t* p_msg_length, const driver_data_t* p_data);
void scenic_ops_del_script(uint32_t* p_msg_length, const driver_data_t* p_data);
void scenic_ops_reset(const driver_data_t* p_data);
void scenic_ops_global_tx(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_cursor_tx(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_render(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_update_cursor(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_clear_color(uint32_t* p_msg_length, const driver_data_t* p_data);
void scenic_ops_quit(driver_data_t* p_data);
void scenic_ops_put_font(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_put_image(uint32_t* p_msg_length, driver_data_t* p_data);
void scenic_ops_crash();

void dispatch_scenic_ops(uint32_t msg_length, driver_data_t* p_data);
void* scenic_loop(void* user_data);