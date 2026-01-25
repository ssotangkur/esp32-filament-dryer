#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  void init_display(void);

  // FPS monitoring functions
  void fps_monitor_start(void);
  void fps_monitor_stop(void);
  uint32_t fps_monitor_get_fps(void);
  void fps_monitor_reset(void);

#ifdef __cplusplus
}
#endif
