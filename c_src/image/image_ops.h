#pragma once

#include <stdint.h>

int32_t image_ops_create(void* v_ctx, uint32_t width, uint32_t height, void* p_pixels);
void image_ops_update(void* v_ctx, uint32_t image_id, void* p_pixels);
void image_ops_delete(void* v_ctx, uint32_t image_id);
