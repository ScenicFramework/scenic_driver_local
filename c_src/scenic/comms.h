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
  MSG_OUT_CLOSE = 0X00,
  MSG_OUT_STATS = 0X01,
  MSG_OUT_PUTS  = 0X02,
  MSG_OUT_WRITE = 0X03,
  MSG_OUT_INSPECT = 0X04,
  MSG_OUT_RESHAPE = 0X05,
  MSG_OUT_READY = 0X06,
  MSG_OUT_DRAW_READY = 0X07,

  MSG_OUT_KEY = 0X0A,
  MSG_OUT_CODEPOINT = 0X0B,
  MSG_OUT_CURSOR_POS = 0X0C,
  MSG_OUT_MOUSE_BUTTON = 0X0D,
  MSG_OUT_MOUSE_SCROLL = 0X0E,
  MSG_OUT_CURSOR_ENTER = 0X0F,
  MSG_OUT_DROP_PATHS = 0X10,
  MSG_OUT_STATIC_TEXTURE_MISS = 0X20,
  MSG_OUT_DYNAMIC_TEXTURE_MISS = 0X21,

  MSG_OUT_FONT_MISS = 0X22,
  MSG_OUT_IMG_MISS = 0X23,

  MSG_OUT_NEW_TX_ID = 0X31,
  MSG_OUT_NEW_FONT_ID = 0X32,

  MSG_OUT_INFO = 0XA0,
  MSG_OUT_WARN = 0XA1,
  MSG_OUT_ERROR = 0XA2,
  MSG_OUT_DEBUG = 0XA3,

  _MSG_OUT_SIZE_ = 0XFFFFFFFF,
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
  log_level_debug,
  log_level_info,
  log_level_warn,
  log_level_error,
} log_level_t;

void log_message(log_level_t level, const char* msg, ...);
void log_debug(const char* msg, ...);
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
void handle_stdio_in(driver_data_t* p_data);

int64_t monotonic_time();
