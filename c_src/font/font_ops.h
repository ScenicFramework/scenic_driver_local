#pragma once

#include <stdint.h>
#include "scenic_types.h"
#include "tommyhashlin.h"

typedef struct {
  int font_id;
  sid_t id;
  data_t blob;
  tommy_hashlin_node node;
} font_t;

int32_t font_ops_create(void* v_ctx, font_t* p_font, uint32_t size);
