#include "lvgl.h"
#include "ui/analog_dial.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Physics update rate: 60fps = ~16.67ms */
#define PHYSICS_UPDATE_PERIOD_MS 16
#define PHYSICS_DT_S (PHYSICS_UPDATE_PERIOD_MS / 1000.0f)

/* Spring-damper physics parameters */
#define SPRING_CONSTANT_K 15.0f    /* Higher = stiffer spring, more overshoot */
#define DAMPING_COEFFICIENT_C 0.8f /* Higher = more damping, less oscillation */
#define NEEDLE_MASS_M 0.1f         /* Mass of the needle (affects inertia) */

/* Convergence threshold - stop physics when close enough */
#define POSITION_TOLERANCE 0.1f /* Degrees */
#define VELOCITY_TOLERANCE 0.5f /* Degrees per second */

struct analog_dial_t
{
  lv_obj_t *container;
  lv_obj_t *scale;
  lv_obj_t *needle_line;
  lv_obj_t *value_label;
  int32_t needle_length;

  /* Physics state - using floats for smooth motion */
  float position;        /* Current needle position */
  float target_position; /* Target position from subject */
  float velocity;        /* Current velocity */

  /* Physics timer for continuous updates */
  lv_timer_t *physics_timer;
};

/**
 * Physics update callback - runs at 60fps
 * Calculates spring-damper forces and updates needle position
 */
static void physics_update_cb(lv_timer_t *timer)
{
  struct analog_dial_t *dial = lv_timer_get_user_data(timer);

  /* Calculate spring force: F_spring = -k × displacement */
  float displacement = dial->target_position - dial->position;
  float spring_force = SPRING_CONSTANT_K * displacement;

  /* Calculate damper force: F_damper = -c × velocity */
  float damper_force = DAMPING_COEFFICIENT_C * dial->velocity;

  /* Net force */
  float net_force = spring_force - damper_force;

  /* Update velocity: v += (F / m) × dt */
  float acceleration = net_force / NEEDLE_MASS_M;
  dial->velocity += acceleration * PHYSICS_DT_S;

  /* Update position: x += v × dt */
  dial->position += dial->velocity * PHYSICS_DT_S;

  /* Update LVGL needle position */
  lv_scale_set_line_needle_value(
      dial->scale,
      dial->needle_line,
      dial->needle_length,
      (int32_t)dial->position);

  /* Check if we've converged to target (close enough and slow enough) */
  // if (fabsf(displacement) < POSITION_TOLERANCE &&
  //     fabsf(dial->velocity) < VELOCITY_TOLERANCE) {
  //   /* Snap to exact target and pause physics timer */
  //   dial->position = dial->target_position;
  //   dial->velocity = 0.0f;
  //   lv_scale_set_line_needle_value(
  //       dial->scale,
  //       dial->needle_line,
  //       dial->needle_length,
  //       (int32_t)dial->position);
  //   lv_timer_pause(dial->physics_timer);
  // }
}

/**
 * Observer callback - triggered when subject value changes
 * Updates target position, value label, and wakes physics timer
 */
static void dial_value_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  struct analog_dial_t *dial = lv_observer_get_user_data(observer);
  float new_value = lv_subject_get_float(subject);

  /* Update target position */
  dial->target_position = new_value;

  /* Update value label with exact emitted value (1 decimal precision) */
  char value_text[16];
  snprintf(value_text, sizeof(value_text), "%.1f", new_value);
  lv_label_set_text(dial->value_label, value_text);

  /* Resume physics timer if it was paused */
  lv_timer_resume(dial->physics_timer);
}

struct analog_dial_t *create_analog_dial(
    lv_obj_t *parent,
    lv_subject_t *subject)
{
  /* Dial is bigger that the window it occupies so we need to wrap it
    in a container and use padding to shift it to the right location.
              ◄──container_width──►

             ┌──────Container──────┐
      ▲ ┌───────────────────────────────┐
      │ │    │                     │    │
      │ │    │                     │    │
      d │    │          +──────────┼────┼────
      i │    │                     │    │  |
      a │    │                     │    │  │
      l │    │                     │    │ Offset
      │ │    └─────────────────────┘    │  │
      d │                               │  ▼
      i │               +───────────────┼────
      a │                               │
      m │                               │
      e │                               │
      t │                               │
      e │                               │
      r │                               │
      │ │                               │
      ▼ └───────────────────────────────┘
          ◄────────dial_diameter────────►

  */

  /* Constants */
  int32_t major_tick_length = 10;
  int32_t major_tick_width = 1;
  int32_t minor_tick_length = 5;
  int32_t minor_tick_width = 1;

  /* Calculate padding values */
  int d2 = ANALOG_DIAL_DIAMETER / 2;
  int c2 = ANALOG_DIAL_CONTAINER_HEIGHT / 2;
  int horiz_padding = (ANALOG_DIAL_CONTAINER_WIDTH - ANALOG_DIAL_DIAMETER) / 2;
  int top_padding = d2 + ANALOG_DIAL_VERT_SHIFT - c2;
  int bottom_padding = d2 - ANALOG_DIAL_VERT_SHIFT - c2;

  struct analog_dial_t *dial = malloc(sizeof(struct analog_dial_t));

  /* Initialize physics state */
  dial->position = 0.0f;
  dial->target_position = 0.0f;
  dial->velocity = 0.0f;

  lv_obj_t *container = lv_obj_create(parent);
  dial->container = container;
  lv_obj_set_style_bg_color(container, lv_color_white(), LV_PART_MAIN);
  // lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_top(container, top_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_left(container, horiz_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_right(container, horiz_padding, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(container, bottom_padding, LV_PART_MAIN);
  lv_obj_set_size(container, ANALOG_DIAL_CONTAINER_WIDTH, ANALOG_DIAL_CONTAINER_HEIGHT);
  lv_obj_set_style_radius(container, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  /* Remove scrollbar */
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

  /* Create round scale */
  lv_obj_t *scale_line = lv_scale_create(container);
  dial->scale = scale_line;

  lv_obj_set_size(scale_line, ANALOG_DIAL_DIAMETER, ANALOG_DIAL_DIAMETER);
  lv_obj_set_style_radius(scale_line, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(scale_line, 0, LV_PART_MAIN);
  lv_obj_set_style_margin_all(scale_line, 0, LV_PART_MAIN);
  /* Set origin of scale to be top middle */
  // lv_obj_set_align(scale_line, LV_ALIGN_TOP_MID);
  lv_obj_center(scale_line);

  lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_OUTER);
  lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scale_line, lv_color_white(), 0);
  lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(scale_line, false, LV_PART_INDICATOR);
  lv_obj_set_style_clip_corner(scale_line, false, LV_PART_MAIN);
  lv_scale_set_draw_ticks_on_top(scale_line, true);

  lv_scale_set_label_show(scale_line, true);

  lv_scale_set_total_tick_count(scale_line, 31);
  lv_scale_set_major_tick_every(scale_line, 5);
  lv_obj_set_style_pad_radial(scale_line, -10, LV_PART_INDICATOR);
  lv_obj_set_style_text_font(scale_line, &lv_font_montserrat_12, LV_PART_INDICATOR);

  lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
  lv_obj_set_style_line_width(scale_line, 1, LV_PART_ITEMS);
  lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(scale_line, 1, LV_PART_INDICATOR);
  /* Rotate labels to match tick angle */
  // lv_obj_set_style_transform_rotation(scale_line, LV_SCALE_LABEL_ROTATE_MATCH_TICKS + 900, LV_PART_INDICATOR);

  lv_scale_set_range(scale_line, ANALOG_DIAL_RANGE_START, ANALOG_DIAL_RANGE_END);

  lv_scale_set_angle_range(scale_line, ANALOG_DIAL_ANGLE_RANGE);
  lv_scale_set_rotation(scale_line, 180 + (180 - ANALOG_DIAL_ANGLE_RANGE) / 2);

  /* Create value label on container - container is fully visible */
  lv_obj_t *value_label = lv_label_create(container);
  dial->value_label = value_label;
  lv_obj_set_style_text_font(value_label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(value_label, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(value_label, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_text_opa(value_label, LV_OPA_COVER, LV_PART_MAIN);
  lv_label_set_text(value_label, "0.0");
  /* Move up from bottom - container is 80px high, place label at visible bottom area */
  lv_obj_align(value_label, LV_ALIGN_BOTTOM_MID, 0, -35);

  lv_obj_t *needle_line = lv_line_create(scale_line);
  dial->needle_line = needle_line;

  lv_obj_set_style_line_width(needle_line, 1, LV_PART_MAIN);
  lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

  dial->needle_length = d2 + major_tick_length;

  /* Create physics timer - initially paused */
  dial->physics_timer = lv_timer_create(physics_update_cb, PHYSICS_UPDATE_PERIOD_MS, dial);
  lv_timer_pause(dial->physics_timer);

  /* Bind observer to subject - initial notification will set target and wake physics */
  lv_subject_add_observer_obj(subject, dial_value_observer_cb, container, dial);

  return dial;
}

void free_analog_dial(struct analog_dial_t *dial)
{
  /* Stop physics timer first */
  if (dial->physics_timer != NULL)
  {
    lv_timer_del(dial->physics_timer);
  }

  /* deletes recursively delete children too but we will still try to delete
     from bottom up explicitly */
  lv_obj_delete(dial->needle_line);
  lv_obj_delete(dial->value_label);
  lv_obj_delete(dial->scale);
  lv_obj_delete(dial->container);
  free(dial);
}
