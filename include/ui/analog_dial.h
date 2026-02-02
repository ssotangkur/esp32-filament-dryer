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
 * Creates an analog dial widget bound to a subject for value updates.
 *
 * The dial automatically updates its needle position when the subject's
 * value changes. The subject must remain valid for the lifetime of the dial
 * and must be initialized with lv_subject_init_float() before calling this
 * function.
 *
 * The dial displays colored bands on the scale:
 * - Green: Values within target range [target_value - target_range/2,
 *   target_value + target_range/2]
 * - Red: Values above target range (target_value + target_range/2 to max_value)
 * - Default: Values below target range use default scale styling
 *
 * @param parent        Parent object for the dial
 * @param subject       Subject to bind to for value updates (float type)
 * @param target_value  The target value for the green band center
 * @param target_range  The total width of the green band (green = target +/-
 *   range/2)
 * @param min_value     Minimum value for the dial scale
 * @param max_value     Maximum value for the dial scale
 * @return              Pointer to the created analog dial structure
 */
  struct analog_dial_t *create_analog_dial(
      lv_obj_t *parent,
      lv_subject_t *subject,
      float target_value,
      float target_range,
      float min_value,
      float max_value);

  /**
   * Frees the memory allocated for an analog dial.
   *
   * The observer associated with the dial is automatically cleaned up when
   * the widget is destroyed.
   *
   * @param dial Pointer to the analog dial structure to free
   */
  void free_analog_dial(struct analog_dial_t *dial);

#ifdef __cplusplus
}
#endif
