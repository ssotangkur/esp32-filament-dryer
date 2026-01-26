#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Opaque structure representing an analog dial
 */

/* Analog dial constants */
#define ANALOG_DIAL_DIAMETER 210
#define ANALOG_DIAL_CONTAINER_WIDTH 160
#define ANALOG_DIAL_CONTAINER_HEIGHT 80
#define ANALOG_DIAL_VERT_SHIFT 90
#define ANALOG_DIAL_ANGLE_RANGE 70
#define ANALOG_DIAL_RANGE_START 0
#define ANALOG_DIAL_RANGE_END 120

  typedef struct analog_dial_t analog_dial_t;

  /**
   * Creates an analog dial widget
   *
   * @param parent Parent object for the dial
   * @return Pointer to the created analog dial structure
   */
  struct analog_dial_t *create_analog_dial(lv_obj_t *parent);

  /**
   * Sets the value of an analog dial
   *
   * @param dial Pointer to the analog dial structure
   * @param value Value to set
   */
  void set_analog_dial_value(struct analog_dial_t *dial, int32_t value);

  /**
   * Frees the memory allocated for an analog dial
   *
   * @param dial Pointer to the analog dial structure to free
   */
  void free_analog_dial(struct analog_dial_t *dial);

#ifdef __cplusplus
}
#endif