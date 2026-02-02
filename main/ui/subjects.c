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
    /* Initialize temperature subject (0-120 degrees) as float */
    lv_subject_init_float(&g_subject_temperature, 75.0f);
    lv_subject_set_min_value_float(&g_subject_temperature, 0.0f);
    lv_subject_set_max_value_float(&g_subject_temperature, 120.0f);

    /* Initialize humidity subject (0-100%) as float */
    lv_subject_init_float(&g_subject_humidity, 50.0f);
    lv_subject_set_min_value_float(&g_subject_humidity, 0.0f);
    lv_subject_set_max_value_float(&g_subject_humidity, 100.0f);

    /* Initialize heater power subject (0-100%) as float */
    lv_subject_init_float(&g_subject_heater_power, 0.0f);
    lv_subject_set_min_value_float(&g_subject_heater_power, 0.0f);
    lv_subject_set_max_value_float(&g_subject_heater_power, 100.0f);

    /* Initialize fan speed subject (0-100%) as float */
    lv_subject_init_float(&g_subject_fan_speed, 0.0f);
    lv_subject_set_min_value_float(&g_subject_fan_speed, 0.0f);
    lv_subject_set_max_value_float(&g_subject_fan_speed, 100.0f);

    /* Initialize system state subject (0=idle) as float */
    lv_subject_init_float(&g_subject_system_state, 0.0f);
    lv_subject_set_min_value_float(&g_subject_system_state, 0.0f);
    lv_subject_set_max_value_float(&g_subject_system_state, 3.0f);
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

void subjects_set_temperature(float temperature)
{
    lvgl_port_lock(0);
    lv_subject_set_float(&g_subject_temperature, temperature);
    lvgl_port_unlock();
}

void subjects_set_humidity(float humidity)
{
    lvgl_port_lock(0);
    lv_subject_set_float(&g_subject_humidity, humidity);
    lvgl_port_unlock();
}

void subjects_set_heater_power(float power)
{
    lvgl_port_lock(0);
    lv_subject_set_float(&g_subject_heater_power, power);
    lvgl_port_unlock();
}

void subjects_set_fan_speed(float speed)
{
    lvgl_port_lock(0);
    lv_subject_set_float(&g_subject_fan_speed, speed);
    lvgl_port_unlock();
}

void subjects_set_system_state(float state)
{
    lvgl_port_lock(0);
    lv_subject_set_float(&g_subject_system_state, state);
    lvgl_port_unlock();
}
