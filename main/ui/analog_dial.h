#pragma once

#include <sdkconfig.h>
#include "lvgl.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque structure representing an analog dial
 */
typedef struct analog_dial_t analog_dial_t;

/**
 * Creates an analog dial widget
 *
 * @param parent Parent object for the dial
 * @param dial_diameter Diameter of the dial
 * @param container_width Width of the container
 * @param container_height Height of the container
 * @param vert_shift Vertical shift of the dial
 * @param angle_range Angle range of the dial
 * @param range_start Start value of the range
 * @param range_end End value of the range
 * @return Pointer to the created analog dial structure
 */
analog_dial_t *create_analog_dial(
    lv_obj_t *parent,
    int dial_diameter,
    int container_width,
    int container_height,
    int vert_shift,
    int angle_range,
    int range_start,
    int range_end);

/**
 * Sets the value of an analog dial
 *
 * @param dial Pointer to the analog dial structure
 * @param value Value to set
 */
void set_analog_dial_value(analog_dial_t *dial, int32_t value);

/**
 * Frees the memory allocated for an analog dial
 *
 * @param dial Pointer to the analog dial structure to free
 */
void free_analog_dial(analog_dial_t *dial);

#ifdef __cplusplus
}
#endif