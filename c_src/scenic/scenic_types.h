/*
#  Created by Boyd Multerer on 12/05/17.
#  Copyright © 2017 Kry10 Limited. All rights reserved.
#
*/

// one unified place for the various structures

#pragma once

#ifndef bool
#include <stdbool.h>
#endif

#include <stdint.h>

#ifndef PACK
  #ifdef _MSC_VER
    #define PACK( __Declaration__ ) \
        __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
  #elif defined(__GNUC__)
    #define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
  #endif
#endif

//---------------------------------------------------------
PACK(typedef struct Vector2f
{
  float x;
  float y;
}) Vector2f;


//---------------------------------------------------------
// the data pointed to by the window private data pointer
typedef struct {
  bool keep_going;
  uint32_t input_flags;
  float last_x;
  float last_y;
  int root_script;
  void* p_tx_ids;
  void* p_fonts;
  void* v_ctx;
  float global_tx[6];
  float cursor_tx[6];
  float cursor_pos[2];
  uint32_t f_show_cursor;
  int debug_mode;
} driver_data_t;


typedef struct {
  // internal data tracking
  int width;
  int height;
  float ratio;
  void* v_ctx;
} device_info_t;

typedef struct {
  // options from the command line
  int debug_mode;
  int debug_fps;
  int layer;
  int global_opacity;
  int antialias;
  int cursor;
  int width;
  int height;
  int resizable;
  char* title;
} device_opts_t;

//---------------------------------------------------------
// combination of a size and location. Do NOT assume the
// p_data can be free'd. It is usually in a larger block
// that was the thing that was allocated.
typedef struct _data_t {
  void* p_data;
  uint32_t size;
} data_t;

typedef data_t sid_t;
