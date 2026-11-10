#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern lv_subject_t g_subject_heater_temp;
extern lv_subject_t g_subject_air_temp;
extern lv_subject_t g_subject_heater_power;
extern lv_subject_t g_subject_fan_speed;
extern lv_subject_t g_subject_system_state;

void subjects_init(void);
void subjects_deinit(void);
void subjects_set_heater_temp(float temperature);
void subjects_set_air_temp(float temperature);
void subjects_set_heater_power(float power);
void subjects_set_fan_speed(float speed);
void subjects_set_system_state(float state);

#ifdef __cplusplus
}
#endif
