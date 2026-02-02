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

/**
 * Thread-safe subject setters
 * These functions use LVGL port locking to ensure thread safety
 * when updating subjects from tasks other than the LVGL task.
 * Always use these when updating subjects from sensor tasks or other FreeRTOS tasks.
 */

/**
 * @brief Set temperature subject value (thread-safe)
 * @param temperature Temperature value in degrees Celsius
 */
void subjects_set_temperature(int32_t temperature);

/**
 * @brief Set humidity subject value (thread-safe)
 * @param humidity Humidity value as percentage (0-100)
 */
void subjects_set_humidity(int32_t humidity);

/**
 * @brief Set heater power subject value (thread-safe)
 * @param power Power level as percentage (0-100)
 */
void subjects_set_heater_power(int32_t power);

/**
 * @brief Set fan speed subject value (thread-safe)
 * @param speed Fan speed as percentage (0-100)
 */
void subjects_set_fan_speed(int32_t speed);

/**
 * @brief Set system state subject value (thread-safe)
 * @param state System state (0=idle, 1=heating, 2=cooling, 3=error)
 */
void subjects_set_system_state(int32_t state);

#ifdef __cplusplus
}
#endif
