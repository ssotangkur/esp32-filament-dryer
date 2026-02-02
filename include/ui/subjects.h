#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * UI Subjects - Observable data sources for the user interface
 *
 * These subjects provide a clean separation between data sources and UI widgets.
 * In the UI simulator, they can be updated by mock data tasks.
 * On the device, they will be updated by actual sensor readings.
 */

/* Temperature dial subject (0-120 range) */
extern lv_subject_t g_subject_temperature;

/* Humidity dial subject (0-100 range) */
extern lv_subject_t g_subject_humidity;

/* Heater power subject (0-100%) */
extern lv_subject_t g_subject_heater_power;

/* Fan speed subject (0-100%) */
extern lv_subject_t g_subject_fan_speed;

/* System state subject (0=idle, 1=heating, 2=cooling, 3=error) */
extern lv_subject_t g_subject_system_state;

/**
 * Initialize all UI subjects
 * Must be called before creating any UI widgets that bind to subjects
 */
void subjects_init(void);

/**
 * Deinitialize all UI subjects
 * Call this before shutting down the UI
 */
void subjects_deinit(void);

#ifdef __cplusplus
}
#endif
