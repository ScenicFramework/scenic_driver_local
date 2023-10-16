#include "comms.h"
#include "device.h"
#include "font.h"
#include "image.h"
#include "scenic_ops.h"
#include "script.h"
#include "utils.h"

extern device_info_t g_device_info;

inline
void scenic_ops_put_script(uint32_t* p_msg_length, const driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  put_script(p_msg_length);
}

inline
void scenic_ops_del_script(uint32_t* p_msg_length, const driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  delete_script(p_msg_length);
}

inline
void scenic_ops_reset(const driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  reset_scripts();
}

inline
void scenic_ops_global_tx(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  set_global_tx(p_msg_length, p_data);
}

inline
void scenic_ops_cursor_tx(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  set_cursor_tx(p_msg_length, p_data);
}

inline
void scenic_ops_render(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  render(p_data);
}

inline
void scenic_ops_update_cursor(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  update_cursor(p_msg_length, p_data);
}

inline
void scenic_ops_clear_color(uint32_t* p_msg_length, const driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  clear_color(p_msg_length);
}

inline
void scenic_ops_quit(driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  receive_quit(p_data);
}

inline
void scenic_ops_put_font(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s", __func__);
  }
  put_font(p_msg_length, p_data->v_ctx);
}

inline
void scenic_ops_put_image(uint32_t* p_msg_length, driver_data_t* p_data)
{
  if (p_data->debug_mode) {
    log_info("%s(*%d,%p)", __func__, *p_msg_length, p_data->v_ctx);
  }
  put_image(p_msg_length, p_data->v_ctx);
}

inline
void scenic_ops_crash()
{
  receive_crash();
}

void dispatch_scenic_ops(uint32_t msg_length, driver_data_t* p_data)
{
  scenic_op_t op;
  read_bytes_down(&op, sizeof(uint32_t), &msg_length);

  switch(op)
  {
  case scenic_op_put_script:
    scenic_ops_put_script(&msg_length, p_data);
    break;
  case scenic_op_del_script:
    scenic_ops_del_script(&msg_length, p_data);
    break;
  case scenic_op_reset:
    scenic_ops_reset(p_data);
    break;
  case scenic_op_global_tx:
    scenic_ops_global_tx(&msg_length, p_data);
    break;
  case scenic_op_cursor_tx:
    scenic_ops_cursor_tx(&msg_length, p_data);
    break;
  case scenic_op_render:
    scenic_ops_render(&msg_length, p_data);
    break;
  case scenic_op_update_cursor:
    scenic_ops_update_cursor(&msg_length, p_data);
    break;
  case scenic_op_clear_color:
    scenic_ops_clear_color(&msg_length, p_data);
    break;
  case scenic_op_quit:
    scenic_ops_quit(p_data);
    break;
  case scenic_op_put_font:
    scenic_ops_put_font(&msg_length, p_data);
    break;
  case scenic_op_put_image:
    scenic_ops_put_image(&msg_length, p_data);
    break;
  case scenic_op_crash:
    scenic_ops_crash();
    break;
  }

  // if there are any bytes left to read in the message, need to get rid of them
  // here...
  if (msg_length > 0)
  {
    log_error("Excess message bytes: %d", msg_length);
    log_error("|      op code: %d", op);
    void* p = malloc(msg_length);
    read_bytes_down(p, msg_length, &msg_length);
    free(p);
  }

  check_gl_error();
}

void* scenic_loop(void* user_data)
{
  driver_data_t* p_data = (driver_data_t*)user_data;
  // signal the app that the window is ready
  send_ready();

  /* Loop until the calling app closes the window */
  while (p_data->keep_going && !isCallerDown()) {
    // check for incoming messages - blocks with a timeout
    handle_stdio_in(p_data);
    device_poll();
  }

  reset_images(p_data->v_ctx);

  device_close(&g_device_info);

  return NULL;
}

