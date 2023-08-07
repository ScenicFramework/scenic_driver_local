/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>

#include "common.h"
#include "comms.h"
#include "font.h"
#include "image.h"
#include "script_ops.h"
#include "script.h"
#include "utils.h"

extern device_opts_t g_opts;

//---------------------------------------------------------
typedef struct _script_t {
  sid_t id;
  data_t script;
  tommy_hashlin_node  node;
} script_t;


// #define HASH_ID(id)  tommy_inthash_u32(id)
#define HASH_ID(id)  tommy_hash_u32( 0, id.p_data, id.size )

tommy_hashlin   scripts = {0};


//---------------------------------------------------------
void init_scripts( void ) {
  // init the hash table
  tommy_hashlin_init( &scripts );
}

//=============================================================================
// internal utilities for working with the script hash

// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void* p_arg, const void* p_obj)
{
  const sid_t* p_id = p_arg;
  const script_t* p_script = p_obj;
  return (p_id->size != p_script->id.size)
    || memcmp(p_id->p_data, p_script->id.p_data, p_id->size);
}

//---------------------------------------------------------
script_t* get_script(sid_t id)
{
  return tommy_hashlin_search(&scripts,
                              _comparator,
                              &id,
                              HASH_ID(id));
}

//---------------------------------------------------------
void do_delete_script(sid_t id)
{
  script_t* p_script = get_script(id);
  if (p_script) {
    if (g_opts.debug_mode) {
      log_debug("%s id:'%.*s'", __func__,
                id.size, id.p_data);
    }

    tommy_hashlin_remove_existing(&scripts,
                                  &p_script->node);
    free(p_script);
  }
}

//---------------------------------------------------------
void put_script(uint32_t* p_msg_length)
{
  // read in the length of the id, which is in the first four bytes
  uint32_t id_length;
  read_bytes_down(&id_length, sizeof(uint32_t), p_msg_length);

  // initialize a record to hold the script
  int struct_size = ALIGN_UP(sizeof(script_t), 8);
  int id_size = ALIGN_UP(id_length, 8);
  int alloc_size = struct_size + id_size + *p_msg_length;
  script_t *p_script = malloc(alloc_size);
  if ( !p_script ) {
    log_error("Unable to allocate script");
    return;
  }

  // initialize the id
  p_script->id.size = id_length;
  p_script->id.p_data = ((void*)p_script) + struct_size;
  read_bytes_down(p_script->id.p_data, id_length, p_msg_length);

  // initialize the data
  p_script->script.size = *p_msg_length;
  p_script->script.p_data = ((void*)p_script) + struct_size + id_size;
  read_bytes_down(p_script->script.p_data, *p_msg_length, p_msg_length);

  // if there is already is a script with the same id, delete it
  do_delete_script(p_script->id);

  if (g_opts.debug_mode) {
    log_debug("%s id:'%.*s'", __func__,
              p_script->id.size, p_script->id.p_data);
  }

  // insert the script into the tommy hash
  tommy_hashlin_insert(&scripts,
                       &p_script->node,
                       p_script,
                       HASH_ID(p_script->id));
}

//---------------------------------------------------------
void delete_script(uint32_t* p_msg_length)
{
  sid_t id;

  // read in the length of the id, which is in the first four bytes
  read_bytes_down(&id.size, sizeof(uint32_t), p_msg_length);

  // create a temporary buffer and read the id into it
  id.p_data = malloc(id.size);
  if (!id.p_data) {
    log_error("Unable to allocate buffer for the id");
    return;
  }

  // read in the body of the id
  read_bytes_down(id.p_data, id.size, p_msg_length);

  // delete and free
  do_delete_script(id);

  free(id.p_data);
}

//---------------------------------------------------------
void reset_scripts() {
  // deallocates all the objects iterating the hashtable
  tommy_hashlin_foreach( &scripts, free );

  // deallocates the hashtable
  tommy_hashlin_done( &scripts );

  // re-init the hash table
  tommy_hashlin_init( &scripts );
}


//=============================================================================
// rendering

static inline uint8_t get_byte(void* p, uint32_t offset)
{
  return *((uint8_t*)(p + offset));
}

static inline uint16_t get_uint16(void* p, uint32_t offset)
{
  return ntoh_ui16(*((uint16_t*)(p + offset)));
}

static inline uint32_t get_uint32(void* p, uint32_t offset)
{
  return ntoh_ui32(*((uint32_t*)(p + offset)));
}

static inline float get_float(void* p, uint32_t offset)
{
  return ntoh_f32(*((float*)(p + offset)));
}


int padded_advance(int size)
{
  switch( size % 4 ) {
    case 0: return size;
    case 1: return size + 3;
    case 2: return size + 2;
    case 3: return size + 1;
    default: return size;
  };
}

//---------------------------------------------------------
void render_script(void* v_ctx, sid_t id)
{
  // get the script
  script_t* p_script = get_script(id);
  if ( !p_script ) {
    return;
  }

  // track the state pushes
  int push_count = 0;

  // setup
  void* p = p_script->script.p_data;
  int i = 0;

  while (i < p_script->script.size) {
    script_op_t op = (script_op_t)get_uint16(p, i);
    uint16_t param = get_uint16(p, i + 2);
    i += 4;

    switch(op) {
      case SCRIPT_OP_DRAW_LINE:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          script_ops_draw_line(v_ctx, a, b, (param & FLAG_STROKE));
        }
        i += 16;
        break;
      case SCRIPT_OP_DRAW_TRIANGLE:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t c = {get_float(p, i + 16), get_float(p, i + 20)};
          script_ops_draw_triangle(v_ctx, a, b, c, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 24;
        break;
      case SCRIPT_OP_DRAW_QUAD:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t c = {get_float(p, i + 16), get_float(p, i + 20)};
          coordinates_t d = {get_float(p, i + 24), get_float(p, i + 28)};
          script_ops_draw_quad(v_ctx, a, b, c, d, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 32;
        break;
      case SCRIPT_OP_DRAW_RECT:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          script_ops_draw_rect(v_ctx, w, h, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 8;
        break;
      case SCRIPT_OP_DRAW_RRECT:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          float radius = get_float(p, i + 8);
          script_ops_draw_rrect(v_ctx, w, h, radius, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 12;
        break;
      case SCRIPT_OP_DRAW_RRECTV:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          float ulr = get_float(p, i + 8);
          float urr = get_float(p, i + 12);
          float lrr = get_float(p, i + 16);
          float llr = get_float(p, i + 20);
          script_ops_draw_rrectv(v_ctx, w, h, ulr, urr, lrr, llr, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 24;
        break;
      case SCRIPT_OP_DRAW_ARC:
        {
          float radius = get_float(p, i);
          float radians = get_float(p, i + 4);
          script_ops_draw_arc(v_ctx, radius, radians, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 8;
        break;
      case SCRIPT_OP_DRAW_SECTOR:
        {
          float radius = get_float(p, i);
          float radians = get_float(p, i + 4);
          script_ops_draw_sector(v_ctx, radius, radians, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 8;
        break;
      case SCRIPT_OP_DRAW_CIRCLE:
        {
          float radius = get_float(p, i);
          script_ops_draw_circle(v_ctx, radius, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 4;
        break;
      case SCRIPT_OP_DRAW_ELLIPSE:
        {
          float radius0 = get_float(p, i);
          float radius1 = get_float(p, i + 4);
          script_ops_draw_ellipse(v_ctx, radius0, radius1, (param & FLAG_FILL), (param & FLAG_STROKE));
        }
        i += 8;
        break;
      case SCRIPT_OP_DRAW_TEXT:
        {
          uint32_t size = param;
          const char* text = p + i;
          script_ops_draw_text(v_ctx, size, text);
        }
        i += padded_advance( param );
        break;
      case SCRIPT_OP_DRAW_SPRITES:
        {
          uint32_t count = get_uint32(p, i);
          sprite_t* sprites = malloc(count * sizeof(sprite_t));

          i += sizeof(uint32_t);

          // get the id
          sid_t id;
          id.size = param;
          id.p_data = p + i;
          i += padded_advance(param);

          // loop the draw commands and draw each
          for (int n = 0; n < count; n++) {
            sprites[n] = (sprite_t){
              .sx = get_float(p, i),
              .sy = get_float(p, i + 4),
              .sw = get_float(p, i + 8),
              .sh = get_float(p, i + 12),
              .dx = get_float(p, i + 16),
              .dy = get_float(p, i + 20),
              .dw = get_float(p, i + 24),
              .dh = get_float(p, i + 28)
            };

            i += 32;
          }
          script_ops_draw_sprites(v_ctx, id, count, sprites);
          free(sprites);
        }
        break;
      case SCRIPT_OP_DRAW_SCRIPT:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_draw_script(v_ctx, id);
        i += padded_advance(param);
        break;
      case SCRIPT_OP_BEGIN_PATH:
        script_ops_begin_path(v_ctx);
        break;
      case SCRIPT_OP_CLOSE_PATH:
        script_ops_close_path(v_ctx);
        break;
      case SCRIPT_OP_FILL_PATH:
        script_ops_fill_path(v_ctx);
        break;
      case SCRIPT_OP_STROKE_PATH:
        script_ops_stroke_path(v_ctx);
        break;
      case SCRIPT_OP_MOVE_TO:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          script_ops_move_to(v_ctx, a);
        }
        i += 8;
        break;
      case SCRIPT_OP_LINE_TO:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          script_ops_line_to(v_ctx, a);
        }
        i += 8;
        break;
      case SCRIPT_OP_ARC_TO:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          float radius = get_float(p, i + 16);
          script_ops_arc_to(v_ctx, a, b, radius);
        }
        i += 20;
        break;
      case SCRIPT_OP_BEZIER_TO:
        {
          coordinates_t c0 = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t c1 = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t a = {get_float(p, i + 16), get_float(p, i + 20)};
          script_ops_bezier_to(v_ctx, c0, c1, a);
        }
        i += 24;
        break;
      case SCRIPT_OP_QUADRATIC_TO:
        {
          coordinates_t c = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t a = {get_float(p, i + 8), get_float(p, i + 12)};
          script_ops_quadratic_to(v_ctx, c, a);
        }
        i += 16;
        break;
      case SCRIPT_OP_ARC:
        {
          coordinates_t c = {get_float(p, i), get_float(p, i + 4)};
          float radius = get_float(p, i + 8);
          float a0 = get_float(p, i + 12);
          float a1 = get_float(p, i + 16);
          sweep_dir_t sweep_dir = get_uint32(p, i + 20);
          script_ops_arc(v_ctx, c, radius, a0, a1, sweep_dir);
        }
        i += 24;
        break;
      case SCRIPT_OP_POP_STATE:
        if (push_count > 0) {
          push_count--;
          script_ops_pop_state(v_ctx);
        }
        break;
      case SCRIPT_OP_POP_PUSH_STATE:
        if (push_count > 0) {
          push_count--;
          script_ops_pop_state(v_ctx);
        }
        // [[fallthrough]];
      case SCRIPT_OP_PUSH_STATE:
        push_count++;
        script_ops_push_state(v_ctx);
        break;

      // case 0x43:        // clear
      case SCRIPT_OP_SCISSOR:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          script_ops_scissor(v_ctx, w, h);
        }
        i += 8;
        break;

      case SCRIPT_OP_TRANSFORM:
        {
          float a = get_float(p, i);
          float b = get_float(p, i + 4);
          float c = get_float(p, i + 8);
          float d = get_float(p, i + 12);
          float e = get_float(p, i + 16);
          float f = get_float(p, i + 20);
          script_ops_transform(v_ctx, a, b, c, d, e, f);
        }
        i += 24;
        break;
      case SCRIPT_OP_SCALE:
        {
          float x = get_float(p, i);
          float y = get_float(p, i + 4);
          script_ops_scale(v_ctx, x, y);
        }
        i += 8;
        break;
      case SCRIPT_OP_ROTATE:
        {
          float radians = get_float(p, i);
          script_ops_rotate(v_ctx, radians);
        }
        i += 4;
        break;
      case SCRIPT_OP_TRANSLATE:
        {
          float x = get_float(p, i);
          float y = get_float(p, i + 4);
          script_ops_translate(v_ctx, x, y);
        }
        i += 8;
        break;
      case SCRIPT_OP_FILL_COLOR:
        {
          color_rgba_t color = {
            get_byte(p,i),
            get_byte(p,i+1),
            get_byte(p,i+2),
            get_byte(p,i+3)
          };
          script_ops_fill_color(v_ctx, color);
        }
        i += 4;
        break;
      case SCRIPT_OP_FILL_LINEAR:
        {
          coordinates_t start = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t end = {get_float(p, i + 8), get_float(p, i + 12)};
          color_rgba_t color_start = {
            get_byte(p, i + 16),
            get_byte(p, i + 17),
            get_byte(p, i + 18),
            get_byte(p, i + 19)
          };
          color_rgba_t color_end = {
            get_byte(p, i + 20),
            get_byte(p, i + 21),
            get_byte(p, i + 22),
            get_byte(p, i + 23)
          };
          script_ops_fill_linear(v_ctx,
                                 start, end,
                                 color_start, color_end);
        }
        i += 24;
        break;
      case SCRIPT_OP_FILL_RADIAL:
        {
          coordinates_t center = {get_float(p, i), get_float(p, i + 4)};
          float inner_radius = get_float(p, i + 8);
          float outer_radius = get_float(p, i + 12);
          color_rgba_t color_start = {
            get_byte(p, i + 16),
            get_byte(p, i + 17),
            get_byte(p, i + 18),
            get_byte(p, i + 19)
          };
          color_rgba_t color_end = {
            get_byte(p, i + 20),
            get_byte(p, i + 21),
            get_byte(p, i + 22),
            get_byte(p, i + 23)
          };
          script_ops_fill_radial(v_ctx, center,
                                 inner_radius,
                                 outer_radius,
                                 color_start, color_end);
        }
        i += 24;
        break;
      case SCRIPT_OP_FILL_IMAGE:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_fill_image(v_ctx, id);
        i += padded_advance(param);
      break;
      case SCRIPT_OP_FILL_STREAM:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_fill_stream(v_ctx, id);
        i += padded_advance(param);
      break;

      case SCRIPT_OP_STROKE_WIDTH:
        {
          float w = param / 4.0;
          script_ops_stroke_width(v_ctx, w);
        }
        break;
      case SCRIPT_OP_STROKE_COLOR:
        {
          color_rgba_t color = {
            get_byte(p, i),
            get_byte(p, i + 1),
            get_byte(p, i + 2),
            get_byte(p, i + 3)
          };
          script_ops_stroke_color(v_ctx, color);
        }
        i += 4;
        break;
      case SCRIPT_OP_STROKE_LINEAR:
        {
          coordinates_t start = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t end = {get_float(p, i + 8), get_float(p, i + 12)};
          color_rgba_t color_start = {
            get_byte(p, i + 16),
            get_byte(p, i + 17),
            get_byte(p, i + 18),
            get_byte(p, i + 19)
          };
          color_rgba_t color_end = {
            get_byte(p, i + 20),
            get_byte(p, i + 21),
            get_byte(p, i + 22),
            get_byte(p, i + 23)
          };
          script_ops_stroke_linear(v_ctx,
                                   start, end,
                                   color_start, color_end);
        }
        i += 24;
        break;
      case SCRIPT_OP_STROKE_RADIAL:
        {
          coordinates_t center = {get_float(p, i), get_float(p, i + 4)};
          float inner_radius = get_float(p, i + 8);
          float outer_radius = get_float(p, i + 12);
          color_rgba_t color_start = {
            get_byte(p, i + 16),
            get_byte(p, i + 17),
            get_byte(p, i + 18),
            get_byte(p, i + 19)
          };
          color_rgba_t color_end = {
            get_byte(p, i + 20),
            get_byte(p, i + 21),
            get_byte(p, i + 22),
            get_byte(p, i + 23)
          };
          script_ops_stroke_radial(v_ctx, center,
                                   inner_radius,
                                   outer_radius,
                                   color_start, color_end);
        }
        i += 24;
        break;
      case SCRIPT_OP_STROKE_IMAGE:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_stroke_image(v_ctx, id);
        i += padded_advance(param);
      break;
      case SCRIPT_OP_STROKE_STREAM:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_stroke_stream(v_ctx, id);
        i += padded_advance(param);
      break;
      case SCRIPT_OP_LINE_CAP:
        {
          line_cap_t line_cap = (line_cap_t)param;
          script_ops_line_cap(v_ctx, line_cap);
        }
        break;
      case SCRIPT_OP_LINE_JOIN:
        {
          line_join_t line_join = (line_join_t)param;
          script_ops_line_join(v_ctx, line_join);
        }
        break;
      case SCRIPT_OP_MITER_LIMIT:
        script_ops_miter_limit(v_ctx, param);
        break;
      case SCRIPT_OP_FONT:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_font(v_ctx, id);
        i += padded_advance(param);
        break;
      case SCRIPT_OP_FONT_SIZE:
        {
          float size = param / 4.0;
          script_ops_font_size(v_ctx, size);
        }
        break;
      case SCRIPT_OP_TEXT_ALIGN:
        {
          text_align_t text_align = (text_align_t)param;
          script_ops_text_align(v_ctx, text_align);
        }
        break;
      case SCRIPT_OP_TEXT_BASE:
        {
          text_base_t text_base = (text_base_t)param;
          script_ops_text_base(v_ctx, text_base);
        }
        break;

      default:
        log_error("Unknown script_op: %d", op);
        break;
    }
  }

  // if there are unbalanced pushes, clear them
  while (push_count > 0) {
    push_count--;
    script_ops_pop_state(v_ctx);
  }
}
