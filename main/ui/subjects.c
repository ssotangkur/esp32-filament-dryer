#include <sdkconfig.h>
#include "ui/subjects.h"
#include "esp_lvgl_port.h"

/* Define all subjects */
lv_subject_t g_subject_temperature;
lv_subject_t g_subject_humidity;
lv_subject_t g_subject_heater_power;
lv_subject_t g_subject_fan_speed;
lv_subject_t g_subject_system_state;

void subjects_init(void)
{
    /* Initialize temperature subject (0-120 degrees) */
    lv_subject_init_int(&g_subject_temperature, 75);
    lv_subject_set_min_value_int(&g_subject_temperature, 0);
    lv_subject_set_max_value_int(&g_subject_temperature, 120);

    /* Initialize humidity subject (0-100%) */
    lv_subject_init_int(&g_subject_humidity, 50);
    lv_subject_set_min_value_int(&g_subject_humidity, 0);
    lv_subject_set_max_value_int(&g_subject_humidity, 100);

    /* Initialize heater power subject (0-100%) */
    lv_subject_init_int(&g_subject_heater_power, 0);
    lv_subject_set_min_value_int(&g_subject_heater_power, 0);
    lv_subject_set_max_value_int(&g_subject_heater_power, 100);

    /* Initialize fan speed subject (0-100%) */
    lv_subject_init_int(&g_subject_fan_speed, 0);
    lv_subject_set_min_value_int(&g_subject_fan_speed, 0);
    lv_subject_set_max_value_int(&g_subject_fan_speed, 100);

    /* Initialize system state subject (0=idle) */
    lv_subject_init_int(&g_subject_system_state, 0);
    lv_subject_set_min_value_int(&g_subject_system_state, 0);
    lv_subject_set_max_value_int(&g_subject_system_state, 3);
}

void subjects_deinit(void)
{
    /* Deinitialize all subjects */
    lv_subject_deinit(&g_subject_temperature);
    lv_subject_deinit(&g_subject_humidity);
    lv_subject_deinit(&g_subject_heater_power);
    lv_subject_deinit(&g_subject_fan_speed);
    lv_subject_deinit(&g_subject_system_state);
}

/*
 * Thread-safe setter implementations
 * These use LVGL port locking to ensure safe access from any task
 */

void subjects_set_temperature(int32_t temperature)
{
    lvgl_port_lock(0);
    lv_subject_set_int(&g_subject_temperature, temperature);
    lvgl_port_unlock();
}

void subjects_set_humidity(int32_t humidity)
{
    lvgl_port_lock(0);
    lv_subject_set_int(&g_subject_humidity, humidity);
    lvgl_port_unlock();
}

void subjects_set_heater_power(int32_t power)
{
    lvgl_port_lock(0);
    lv_subject_set_int(&g_subject_heater_power, power);
    lvgl_port_unlock();
}

void subjects_set_fan_speed(int32_t speed)
{
    lvgl_port_lock(0);
    lv_subject_set_int(&g_subject_fan_speed, speed);
    lvgl_port_unlock();
}

void subjects_set_system_state(int32_t state)
{
    lvgl_port_lock(0);
    lv_subject_set_int(&g_subject_system_state, state);
    lvgl_port_unlock();
}
