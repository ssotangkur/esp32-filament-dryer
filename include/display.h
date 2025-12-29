#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif

  void init_display(void);
  void lvgl_demo(void);

  // FPS monitoring functions
  void fps_monitor_start(void);
  void fps_monitor_stop(void);
  uint32_t fps_monitor_get_fps(void);
  void fps_monitor_reset(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
