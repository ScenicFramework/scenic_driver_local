/*
#  Created by Boyd Multerer on 11/18/17.
#  Copyright © 2017 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>

#ifndef bool
#include <stdbool.h>
#endif

#ifdef _MSC_VER
  #include "windows_comms.h"
#else
  #include "unix_comms.h"
#endif


// ntoh means network-to-host endianness
// hton means host-to-network endianness
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define ntoh_ui16(x) (x)
  #define ntoh_ui32(x) (x)
  #define ntoh_f32(x) (x)
  #define hton_ui16(x) (x)
  #define hton_ui32(x) (x)
  #define hton_f32(x) (x)
#else
// https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
  #define ntoh_ui16(x) (((x) >> 8) | ((x) << 8))
  #define ntoh_ui32(x) \
    (((x) >> 24) | (((x) &0x00FF0000) >> 8) | (((x) &0x0000FF00) << 8) | ((x) << 24))
  static inline float ntoh_f32( float f ) {
    union {
      unsigned int i;
      float f;
    } sw;
    sw.f = f;
    sw.i = ntoh_ui32(sw.i);
    return sw.f;
  }
  #define hton_ui16(x) (ntoh_ui16(x))
  #define hton_ui32(x) (ntoh_ui32(x))
  #define hton_f32(x) (ntoh_f32(x))
#endif


#define INPUT_KEY_MASK 0x0001
#define INPUT_CODEPOINT_MASK 0x0002
#define INPUT_CURSOR_POS_MASK 0x0004
#define INPUT_CURSOR_BUTTON_MASK 0x0008
#define INPUT_CURSOR_SCROLL_MASK 0x0010
#define INPUT_CURSOR_ENTER_MASK 0x0020

typedef enum {
  msg_out_close = 0x00,
  msg_out_stats = 0x01,
  msg_out_puts  = 0x02,
  msg_out_write = 0x03,
  msg_out_inspect = 0x04,
  msg_out_reshape = 0x05,
  msg_out_ready = 0x06,
  msg_out_draw_ready = 0x07,

  msg_out_key = 0x0a,
  msg_out_codepoint = 0x0b,
  msg_out_cursor_pos = 0x0c,
  msg_out_mouse_button = 0x0d,
  msg_out_mouse_scroll = 0x0e,
  msg_out_cursor_enter = 0x0f,
  msg_out_drop_paths = 0x10,
  msg_out_static_texture_miss = 0x20,
  msg_out_dynamic_texture_miss = 0x21,

  msg_out_font_miss = 0x22,
  msg_out_img_miss = 0x23,

  msg_out_new_tx_id = 0x31,
  msg_out_new_font_id = 0x32,

  msg_out_info = 0xa0,
  msg_out_warn = 0xa1,
  msg_out_error = 0xa2,

  _msg_out_SIZE_ = 0xffffffff,
} msg_out_t;

int read_exact(byte* buf, int len);
int write_exact(byte* buf, int len);
int read_msg_length(struct timeval * ptv);
bool isCallerDown();

bool read_bytes_down(void* p_buff, int bytes_to_read,
                     int* p_bytes_to_remaining);

// basic events to send up to the caller
void send_puts(const char* msg, ...);
void send_write(const char* msg);
void send_inspect(void* data, int length);

typedef enum {
  log_level_info,
  log_level_warn,
  log_level_error,
} log_level_t;

void log_message(log_level_t level, const char* msg, ...);
void log_info(const char* msg, ...);
void log_warn(const char* msg, ...);
void log_error(const char* msg, ...);

void set_global_tx(int* p_msg_length, driver_data_t* p_data);
void set_cursor_tx(int* p_msg_length, driver_data_t* p_data);
void update_cursor(int* p_msg_length, driver_data_t* p_data);
void clear_color(int* p_msg_length);
void receive_crash();
void receive_quit(driver_data_t* p_data);
void render(driver_data_t* p_data);

void send_image_miss(unsigned int img_id);

// void send_static_texture_miss(const char* key);
// void send_dynamic_texture_miss(const char* key);
// void send_font_miss(const char* key);
void send_reshape( int window_width, int window_height );
void send_key(int key, int scancode, int action, int mods);
void send_codepoint(unsigned int codepoint, int mods);
void send_cursor_pos(float xpos, float ypos);
void send_mouse_button(int button, int action, int mods, float xpos,
                       float ypos);
void send_scroll(float xoffset, float yoffset, float xpos, float ypos);
void send_cursor_enter(int entered, float xpos, float ypos);
void send_close( int reason );
void send_ready();
// void send_draw_ready(unsigned int id);


// void* comms_thread(void* window);
void handle_stdio_in(driver_data_t* p_data);

int64_t monotonic_time();
