#include "comms.h"
#include "image_ops.h"

__attribute__((weak))
int32_t image_ops_create(void* v_ctx, uint32_t width, uint32_t height, void* p_pixels)
{
  log_error("Unimplemented %s", __func__);
  log_error("|    width: %d", width);
  log_error("|    height: %d", height);
  return -1;
}

__attribute__((weak))
void image_ops_update(void* v_ctx, uint32_t image_id, void* p_pixels)
{
  log_error("Unimplemented %s", __func__);
  log_error("|    image_id: %d", image_id);
}

__attribute__((weak))
void image_ops_delete(void* v_ctx, uint32_t image_id)
{
  log_error("Unimplemented %s", __func__);
  log_error("|    image_id: %d", image_id);
}
