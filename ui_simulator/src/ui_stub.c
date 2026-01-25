// Stub implementations for hardware-specific functions used in UI code
// These are used when building for the simulator to avoid dependencies on ESP-IDF

#ifdef BUILD_FOR_SIMULATOR

#include "ui_stub.h"
#include <stdio.h>

// Stub for lvgl_port_lock - does nothing in simulator
void lvgl_port_lock(int timeout_ms)
{
  // In the simulator, we don't need to lock since there's no hardware access
  // Just a placeholder to satisfy the function call
  (void)timeout_ms; // Suppress unused parameter warning
}

// Stub for lvgl_port_unlock - does nothing in simulator
void lvgl_port_unlock()
{
  // In the simulator, we don't need to unlock since there's no hardware access
  // Just a placeholder to satisfy the function call
}

#endif // BUILD_FOR_SIMULATOR