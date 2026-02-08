/**
 * @file mock_data_task.c
 * Mock data generator for UI simulator
 *
 * This file provides simulated sensor data for testing the UI
 * in the simulator environment. It periodically updates the
 * subjects with random values that mimic real sensor readings.
 */

#include <stdlib.h>
#include <time.h>
#include "lvgl.h"
#include "ui/subjects.h"

/* Mock data update interval in milliseconds */
#define MOCK_DATA_UPDATE_INTERVAL_MS 500

/* Random value range for heater temperature (20-110 degrees) */
#define HEATER_TEMP_MIN 20.0f
#define HEATER_TEMP_MAX 110.0f

/* Random value range for air temperature (20-110 degrees) */
#define AIR_TEMP_MIN 20.0f
#define AIR_TEMP_MAX 110.0f

/* Current mock values */
static float mock_heater_temp = 75.0f;
static float mock_air_temp = 75.0f;

/**
 * Generate a random value within a range with smooth transitions
 */
static float generate_smooth_random(float current, float min, float max, float max_change)
{
    /* Calculate max possible changes */
    float change = (float)((rand() % (int)(max_change * 2.0f * 10.0f + 1)) - (int)(max_change * 10.0f)) / 10.0f;
    float new_value = current + change;

    /* Clamp to range */
    if (new_value < min) new_value = min;
    if (new_value > max) new_value = max;

    return new_value;
}

/**
 * Timer callback to update mock data
 */
static void mock_data_update_cb(lv_timer_t *timer)
{
    (void)timer;

    /* Update heater temperature with smooth random walk (max change of 3 degrees) */
    mock_heater_temp = generate_smooth_random(mock_heater_temp, HEATER_TEMP_MIN, HEATER_TEMP_MAX, 3.0f);
    lv_subject_set_float(&g_subject_heater_temp, mock_heater_temp);

    /* Update air temperature with smooth random walk (max change of 3 degrees) */
    mock_air_temp = generate_smooth_random(mock_air_temp, AIR_TEMP_MIN, AIR_TEMP_MAX, 3.0f);
    lv_subject_set_float(&g_subject_air_temp, mock_air_temp);
}

/**
 * Initialize the mock data generator
 * Call this after subjects_init() and before the main loop
 */
void mock_data_init(void)
{
    /* Seed random number generator */
    srand((unsigned int)time(NULL));

    /* Create timer to periodically update mock data */
    lv_timer_create(mock_data_update_cb, MOCK_DATA_UPDATE_INTERVAL_MS, NULL);
}
