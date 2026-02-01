#pragma once

#include "lvgl.h"

static inline bool lvgl_port_lock(uint32_t timeout_ms)
{
  return true;
}

static inline void lvgl_port_unlock(void) {
  // no-op
};