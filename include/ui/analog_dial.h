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
 * @param parent  Parent object for the dial
 * @param subject Subject to bind to for value updates (float type)
 * @return        Pointer to the created analog dial structure
 */
  struct analog_dial_t *create_analog_dial(lv_obj_t *parent, lv_subject_t *subject);

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
