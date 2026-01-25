#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  void print_memory_info(void);
  void print_fps_info(void);

  // FPS monitoring functions
  void fps_monitor_start(void);
  void fps_monitor_stop(void);
  uint32_t fps_monitor_get_fps(void);
  void fps_monitor_reset(void);
  void fps_monitor_setup_callback(void *disp);

#ifdef __cplusplus
}
#endif
