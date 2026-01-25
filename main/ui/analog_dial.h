#pragma once

#include <sdkconfig.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Opaque structure representing an analog dial
     */
    typedef struct analog_dial_t;

/* Analog dial constants */
#define ANALOG_DIAL_DIAMETER 210
#define ANALOG_DIAL_CONTAINER_WIDTH 160
#define ANALOG_DIAL_CONTAINER_HEIGHT 80
#define ANALOG_DIAL_VERT_SHIFT 90
#define ANALOG_DIAL_ANGLE_RANGE 70
#define ANALOG_DIAL_RANGE_START 0
#define ANALOG_DIAL_RANGE_END 120

    /**
     * Creates an analog dial widget
     *
     * @param parent Parent object for the dial
     * @return Pointer to the created analog dial structure
     */
    analog_dial_t *create_analog_dial(lv_obj_t *parent);

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
