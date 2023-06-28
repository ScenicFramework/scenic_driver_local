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
#include "ops/script_ops.h"
#include "script.h"
#include "utils.h"


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
void do_delete_script( sid_t id )
{
  script_t* p_script = get_script(id);
  if (p_script) {
    tommy_hashlin_remove_existing(&scripts,
                                  &p_script->node);
    free(p_script);
  }
}

//---------------------------------------------------------
void put_script(int* p_msg_length)
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

  // insert the script into the tommy hash
  tommy_hashlin_insert(&scripts,
                       &p_script->node,
                       p_script,
                       HASH_ID(p_script->id));
}

//---------------------------------------------------------
void delete_script(int* p_msg_length)
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

static void put_msg_i( char* msg, int n ) {
  char buff[200];
  sprintf(buff, "%s %d", msg, n);
  send_puts(buff);
}

static inline byte get_byte( void* p, uint32_t offset ) {
  return *((byte*)(p + offset));
}

static inline unsigned short get_uint16( void* p, uint32_t offset ) {
  return ntoh_ui16(*((unsigned short*)(p + offset)));
}

static inline unsigned int get_uint32( void* p, uint32_t offset ) {
  return ntoh_ui32(*((unsigned int*)(p + offset)));
}

static inline float get_float( void* p, uint32_t offset ) {
  return ntoh_f32(*((float*)(p + offset)));
}


int padded_advance( int size ) {
  switch( size % 4 ) {
    case 0: return size;
    case 1: return size + 3;
    case 2: return size + 2;
    case 3: return size + 1;
    default: size;
  };
}



/*
//---------------------------------------------------------
void set_font( sid_t id, NVGcontext* p_ctx ) {
  // unfortunately, nvgFindFont expects a zero terminated C string
  char* p_font = calloc( 1, id.size + 1 );
  if ( !p_font ) {
    send_puts( "Unable to alloc temp font id buffer" );
    return;
  }
  memcpy( p_font, id.p_data, id.size );

  int font_id = nvgFindFont(p_ctx, p_font);
  if (font_id >= 0) {
    nvgFontFaceId(p_ctx, font_id);
  }

  free( p_font );
}
*/

//---------------------------------------------------------
void render_script(void* v_ctx, sid_t id)
{
  log_info("%s(%s)", __func__, id.p_data);
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
      case script_op_draw_line:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          script_ops_draw_line(v_ctx, a, b, (param & flag_fill));
        }
        i += 16;
        break;
      case script_op_draw_triangle:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t c = {get_float(p, i + 16), get_float(p, i + 20)};
          script_ops_draw_triangle(v_ctx, a, b, c, (param & flag_fill), (param & flag_stroke));
        }
        i += 24;
        break;
      case script_op_draw_quad:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t c = {get_float(p, i + 16), get_float(p, i + 20)};
          coordinates_t d = {get_float(p, i + 24), get_float(p, i + 28)};
          script_ops_draw_quad(v_ctx, a, b, c, d, (param & flag_fill), (param & flag_stroke));
        }
        i += 32;
        break;
      case script_op_draw_rect:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          script_ops_draw_rect(v_ctx, w, h, (param & flag_fill), (param & flag_stroke));
        }
        i += 8;
        break;
      case script_op_draw_rrect:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          float radius = get_float(p, i + 8);
          script_ops_draw_rrect(v_ctx, w, h, radius, (param & flag_fill), (param & flag_stroke));          
        }
        i += 12;
        break;
      case script_op_draw_arc:
        {
          float radius = get_float(p, i);
          float radians = get_float(p, i + 4);
          script_ops_draw_arc(v_ctx, radius, radians, (param & flag_fill), (param & flag_stroke));          
        }
        i += 8;
        break;
      case script_op_draw_sector:
        {
          float radius = get_float(p, i);
          float radians = get_float(p, i + 4);
          script_ops_draw_sector(v_ctx, radius, radians, (param & flag_fill), (param & flag_stroke));          
        }
        i += 8;
        break;
      case script_op_draw_circle:
        {
          float radius = get_float(p, i);
          script_ops_draw_circle(v_ctx, radius, (param & flag_fill), (param & flag_stroke));          
        }
        i += 4;
        break;
      case script_op_draw_ellipse:
        {
          float radius0 = get_float(p, i);
          float radius1 = get_float(p, i + 4);
          script_ops_draw_ellipse(v_ctx, radius0, radius1, (param & flag_fill), (param & flag_stroke));          
        }
        i += 8;
        break;
      case script_op_draw_text:
        {
          uint32_t size = param;
          const char* text = p + i;
          script_ops_draw_text(v_ctx, size, text);
        }
        i += padded_advance( param );
        break;
      case script_op_draw_sprites:
        {
          uint32_t count = get_uint32(p, i);
          sprite_t* sprites = malloc(count * sizeof(sprites));

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
      case script_op_draw_script:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_draw_script(v_ctx, id);
        i += padded_advance(param);
        break;
      case script_op_begin_path:
        script_ops_begin_path(v_ctx);
        break;
      case script_op_close_path:
        script_ops_close_path(v_ctx);
        break;
      case script_op_fill_path:
        script_ops_fill_path(v_ctx);
        break;
      case script_op_stroke_path:
        script_ops_stroke_path(v_ctx);
        break;
      case script_op_move_to:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          script_ops_move_to(v_ctx, a);
        }
        i += 8;
        break;
      case script_op_line_to:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          script_ops_line_to(v_ctx, a);
        }
        i += 8;
        break;
      case script_op_arc_to:
        {
          coordinates_t a = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t b = {get_float(p, i + 8), get_float(p, i + 12)};
          float radius = get_float(p, i + 16);
          script_ops_arc_to(v_ctx, a, b, radius);
        }
        i += 20;
        break;
      case script_op_bezier_to:
        {
          coordinates_t c0 = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t c1 = {get_float(p, i + 8), get_float(p, i + 12)};
          coordinates_t a = {get_float(p, i + 16), get_float(p, i + 20)};
          script_ops_bezier_to(v_ctx, c0, c1, a);
        }
        i += 24;
        break;
      case script_op_quadratic_to:
        {
          coordinates_t c = {get_float(p, i), get_float(p, i + 4)};
          coordinates_t a = {get_float(p, i + 8), get_float(p, i + 12)};
          script_ops_quadratic_to(v_ctx, c, a);
        }
        i += 16;
        break;
      case script_op_pop_state:
        if (push_count > 0) {
          push_count--;
          script_ops_pop_state(v_ctx);
        }
        break;
      case script_op_pop_push_state:
        if (push_count > 0) {
          push_count--;
          script_ops_pop_state(v_ctx);
        }
        // [[fallthrough]];
      case script_op_push_state:
        push_count++;
        script_ops_push_state(v_ctx);
        break;

      // case 0x43:        // clear
      case script_op_scissor:
        {
          float w = get_float(p, i);
          float h = get_float(p, i + 4);
          script_ops_scissor(v_ctx, w, h);
        }
        i += 8;
        break;

      case script_op_transform:
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
      case script_op_scale:
        {
          float x = get_float(p, i);
          float y = get_float(p, i + 4);
          script_ops_scale(v_ctx, x, y);
        }
        i += 8;
        break;
      case script_op_rotate:
        {
          float radians = get_float(p, i);
          script_ops_rotate(v_ctx, radians);
        }
        i += 4;
        break;
      case script_op_translate:
        {
          float x = get_float(p, i);
          float y = get_float(p, i + 4);
          script_ops_translate(v_ctx, x, y);
        }
        i += 8;
        break;
      case script_op_fill_color:
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
      case script_op_fill_linear:
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
      case script_op_fill_radial:
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
      case script_op_fill_image:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_fill_image(v_ctx, id);
        i += padded_advance(param);
      break;
      case script_op_fill_stream:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_fill_stream(v_ctx, id);
        i += padded_advance(param);
      break;

      case script_op_stroke_width:
        {
          float w = param / 4.0;
          script_ops_stroke_width(v_ctx, w);
        }
        break;
      case script_op_stroke_color:
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
      case script_op_stroke_linear:
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
      case script_op_stroke_radial:
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
      case script_op_stroke_image:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_stroke_image(v_ctx, id);
        i += padded_advance(param);
      break;
      case script_op_stroke_stream:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_stroke_stream(v_ctx, id);
        i += padded_advance(param);
      break;
      case script_op_line_cap:
        {
          line_cap_t line_cap = (line_cap_t)param;
          script_ops_line_cap(v_ctx, line_cap);
        }
        break;
      case script_op_line_join:
        {
          line_join_t line_join = (line_join_t)param;
          script_ops_line_join(v_ctx, line_join);
        }
        break;
      case script_op_miter_limit:
        script_ops_miter_limit(v_ctx, param);
        break;
      case script_op_font:
        // we can reuse the passed in id struct
        id.size = param;
        id.p_data = p + i;
        script_ops_font(v_ctx, id);
        i += padded_advance(param);
        break;
      case script_op_font_size:
        {
          float size = param / 4.0;
          script_ops_font_size(v_ctx, size);
        }
        break;
      case script_op_text_align:
        {
          text_align_t text_align = (text_align_t)param;
          script_ops_text_align(v_ctx, text_align);
        }
        break;
      case script_op_text_base:
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
