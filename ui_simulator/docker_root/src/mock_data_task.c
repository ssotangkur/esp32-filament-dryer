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

/* Random value range for temperature (20-110 degrees) */
#define TEMP_MIN 20
#define TEMP_MAX 110

/* Random value range for humidity (30-80%) */
#define HUMIDITY_MIN 30
#define HUMIDITY_MAX 80

/* Current mock values */
static int32_t mock_temperature = 75;
static int32_t mock_humidity = 50;

/**
 * Generate a random value within a range with smooth transitions
 */
static int32_t generate_smooth_random(int32_t current, int32_t min, int32_t max, int32_t max_change)
{
    /* Calculate max possible changes */
    int32_t change = (rand() % (max_change * 2 + 1)) - max_change;
    int32_t new_value = current + change;

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

    /* Update temperature with smooth random walk (max change of 3 degrees) */
    mock_temperature = generate_smooth_random(mock_temperature, TEMP_MIN, TEMP_MAX, 3);
    lv_subject_set_int(&g_subject_temperature, mock_temperature);

    /* Update humidity with smooth random walk (max change of 5%) */
    mock_humidity = generate_smooth_random(mock_humidity, HUMIDITY_MIN, HUMIDITY_MAX, 5);
    lv_subject_set_int(&g_subject_humidity, mock_humidity);
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
