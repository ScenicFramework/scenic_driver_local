/*
#  Created by Boyd Multerer on 2/14/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#

Functions to facilitate messages coming up or down from the all via stdin
The caller will typically be erlang, so use the 2-byte length indicator
*/

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "device_ops.h"
#include "font.h"
#include "image.h"
#include "ops/scenic_ops.h"
#include "script.h"
#include "utils.h"

// handy time definitions in microseconds
#define MILLISECONDS_8 8000
#define MILLISECONDS_16 16000
#define MILLISECONDS_20 20000
#define MILLISECONDS_32 32000
#define MILLISECONDS_64 64000
#define MILLISECONDS_128 128000

// Setting the timeout too high means input will be laggy as you
// are starving the input polling. Setting it too low means using
// energy for no purpose. Probably best if set similar to the
// frame rate of the application
#define STDIO_TIMEOUT MILLISECONDS_32

extern device_info_t g_device_info;


//=============================================================================
// raw comms with host app
// from erl_comm.c
// http://erlang.org/doc/tutorial/c_port.html#id64377

//---------------------------------------------------------
// the length indicator from erlang is always big-endian
int write_cmd(byte* buf, unsigned int len)
{
  int written = 0;

  uint32_t cmd_len = len;
  cmd_len = hton_ui32(cmd_len);
  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  written = write_exact(buf, len);

  return written;
}

//---------------------------------------------------------
bool read_bytes_down(void* p_buff, int bytes_to_read, int* p_bytes_to_remaining)
{
  if (p_bytes_to_remaining <= 0)
    return false;

  if (bytes_to_read > *p_bytes_to_remaining)
  {
    // read in the remaining bytes
    read_exact(p_buff, *p_bytes_to_remaining);
    *p_bytes_to_remaining = 0;
    // return false
    return false;
  }

  // read in the requested bytes
  read_exact(p_buff, bytes_to_read);
  // do accounting on the bytes remaining
  *p_bytes_to_remaining -= bytes_to_read;
  return true;
}

//=============================================================================
// send messages up to caller

//---------------------------------------------------------
void logv(uint32_t cmd, const char* msg, va_list args)
{
  char* output;
  uint32_t msg_len = vasprintf(&output, msg, args);
  uint32_t cmd_len = ntoh_ui32(msg_len + sizeof(uint32_t));

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) output, msg_len);
  free(output);
}

void send_puts(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  logv(msg_out_puts, msg, args);
}

//---------------------------------------------------------
void log_message(log_level_t level, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);
  uint32_t cmd = msg_out_error;
  switch(level) {
  case log_level_info: cmd = msg_out_info; break;
  case log_level_warn: cmd = msg_out_warn; break;
  case log_level_error: cmd = msg_out_error; break;
  }

  logv(cmd, msg, args);
}
//---------------------------------------------------------
void log_info(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  logv(msg_out_info, msg, args);
}

//---------------------------------------------------------
void log_warn(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  logv(msg_out_warn, msg, args);
}

//---------------------------------------------------------
void log_error(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);

  logv(msg_out_error, msg, args);
}

//---------------------------------------------------------
void send_write(const char* msg)
{
  uint32_t msg_len = strlen(msg);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = msg_out_write;

  cmd_len = ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) msg, msg_len);
}

//---------------------------------------------------------
void send_inspect(void* data, int length)
{
  uint32_t cmd_len = length + sizeof(uint32_t);
  uint32_t cmd     = msg_out_inspect;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact(data, length);
}

//---------------------------------------------------------
void send_static_texture_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = msg_out_static_texture_miss;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
void send_dynamic_texture_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = msg_out_dynamic_texture_miss;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
void send_font_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = msg_out_font_miss;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
PACK(typedef struct msg_reshape_t
{
  uint32_t msg_id;
  uint32_t window_width;
  uint32_t window_height;
}) msg_reshape_t;

void send_reshape( int window_width, int window_height )
{
  msg_reshape_t msg = { msg_out_reshape, window_width, window_height };
  write_cmd((byte*) &msg, sizeof(msg_reshape_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_key_t
{
  uint32_t msg_id;
  uint32_t key;
  uint32_t scancode;
  uint32_t action;
  uint32_t mods;
}) msg_key_t;

void send_key(int key, int scancode, int action, int mods)
{
  msg_key_t msg = {msg_out_key, key, scancode, action, mods};
  write_cmd((byte*) &msg, sizeof(msg_key_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_codepoint_t
{
  uint32_t msg_id;
  uint32_t codepoint;
  uint32_t mods;
}) msg_codepoint_t;

void send_codepoint(unsigned int codepoint, int mods)
{
  msg_codepoint_t msg = {msg_out_codepoint, codepoint, mods};
  write_cmd((byte*) &msg, sizeof(msg_codepoint_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_cursor_pos_t
{
  uint32_t msg_id;
  float    x;
  float    y;
}) msg_cursor_pos_t;

void send_cursor_pos(float xpos, float ypos)
{
  msg_cursor_pos_t msg = {msg_out_cursor_pos, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_pos_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_mouse_button_t
{
  uint32_t msg_id;
  uint32_t button;
  uint32_t action;
  uint32_t mods;
  float    xpos;
  float    ypos;
}) msg_mouse_button_t;

void send_mouse_button(int button, int action, int mods, float xpos, float ypos)
{
  msg_mouse_button_t msg = {
    msg_out_mouse_button,
    button,
    action,
    mods,
    xpos,
    ypos
  };
  write_cmd((byte*) &msg, sizeof(msg_mouse_button_t));
}
  // #define hton_ui16(x) (ntoh_ui16(x))
  // #define hton_ui32(x) (ntoh_ui32(x))
  // #define hton_f32(x) (ntoh_f32(x))

//---------------------------------------------------------
PACK(typedef struct msg_scroll_t
{
  uint32_t msg_id;
  float    x_offset;
  float    y_offset;
  float    x;
  float    y;
}) msg_scroll_t;

void send_scroll(float xoffset, float yoffset, float xpos, float ypos)
{
  msg_scroll_t msg = {msg_out_mouse_scroll, xoffset, yoffset, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_scroll_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_cursor_enter_t
{
  uint32_t msg_id;
  int32_t  entered;
  float    x;
  float    y;
}) msg_cursor_enter_t;

void send_cursor_enter(int entered, float xpos, float ypos)
{
  msg_cursor_enter_t msg = {msg_out_cursor_enter, entered, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_enter_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_close_t
{
  uint32_t msg_id;
  uint32_t reaspn;
}) msg_close_t;

void send_close( int reason )
{
  msg_close_t msg = { msg_out_close, reason };
  write_cmd((byte*) &msg, sizeof(msg_close_t));
}

//---------------------------------------------------------
PACK(typedef struct img_miss_t
{
  uint32_t msg_id;
  uint32_t img_id;
}) img_miss_t;

void send_image_miss( unsigned int img_id )
{
  img_miss_t msg = { msg_out_img_miss, img_id };
  write_cmd((byte*) &msg, sizeof(img_miss_t));
}

//---------------------------------------------------------
void send_ready()
{
  uint32_t msg_id = msg_out_ready;
  write_cmd((byte*) &msg_id, sizeof(msg_id));
}

//---------------------------------------------------------
void receive_quit(driver_data_t* p_data)
{
  // clear the keep_going control flag, this ends the main thread loop
  p_data->keep_going = false;
}

//---------------------------------------------------------
void receive_crash()
{
  log_error("receive_crash - exit");
  exit(EXIT_FAILURE);
}


//---------------------------------------------------------
void render(driver_data_t* p_data)
{
  // prep the id to the root scene
  sid_t id;
  id.p_data = "_root_";
  id.size = strlen(id.p_data);

  // render the scene
  device_ops_begin_render(p_data);

  // render the root script
  render_script(p_data->v_ctx, id);

  // render the cursor if one is provided
  if (p_data->f_show_cursor) {
    device_ops_begin_cursor_render(p_data);

    id.p_data = "_cursor_";
    id.size = strlen(id.p_data);
    render_script(p_data->v_ctx, id);
  }

  device_ops_end_render(p_data);

  // all done
  send_ready();
}

//---------------------------------------------------------
void set_global_tx(int* p_msg_length, driver_data_t* p_data)
{
  for (int i = 0; i < 6; i++) {
    read_bytes_down(&p_data->global_tx[i], sizeof(float), p_msg_length);
  }
}

//---------------------------------------------------------
void set_cursor_tx(int* p_msg_length, driver_data_t* p_data)
{
  for (int i = 0; i < 6; i++) {
    read_bytes_down(&p_data->cursor_tx[i], sizeof(float), p_msg_length);
  }
}

//---------------------------------------------------------
void update_cursor(int* p_msg_length, driver_data_t* p_data)
{
  read_bytes_down(&p_data->f_show_cursor, sizeof(uint32_t), p_msg_length);
  for (int i = 0; i < 2; i++) {
    read_bytes_down(&p_data->cursor_pos[i], sizeof(float), p_msg_length);
  }
}

//---------------------------------------------------------
void clear_color(int* p_msg_length)
{
  byte r, g, b, a;
  read_bytes_down(&r, 1, p_msg_length);
  read_bytes_down(&g, 1, p_msg_length);
  read_bytes_down(&b, 1, p_msg_length);
  read_bytes_down(&a, 1, p_msg_length);
  device_ops_clear_color(r / 255.0f,
                         g / 255.0f,
                         b / 255.0f,
                         a / 255.0f);
}

//=============================================================================
// non-threaded command reading

int64_t monotonic_time()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t mt_msecs = (int64_t)(ts.tv_sec) * 1000;
    double temp = ((double)(ts.tv_nsec) / 1000000.0) + 0.5;
    mt_msecs += (int64_t)temp;
    return mt_msecs;
}

// read from the stdio in buffer and act on one message.
void handle_stdio_in(driver_data_t* p_data)
{
  int64_t start = monotonic_time();
  int64_t time_remaining = STDIO_TIMEOUT;

  struct timeval tv;
  while (time_remaining > 0) {
    tv.tv_sec  = 0;
    tv.tv_usec = time_remaining;

    int len = read_msg_length(&tv);
    if (len <= 0) break;

    // process the message
    dispatch_scenic_ops(len, p_data);

    // see if time is remaining, so we can process another one
    time_remaining -= monotonic_time() - start;
  }
}
