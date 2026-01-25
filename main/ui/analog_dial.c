#include <sdkconfig.h>
#include "lvgl.h"
#include "analog_dial.h"
#include <stdlib.h>

struct analog_dial_t
{
  lv_obj_t *container;
  lv_obj_t *scale;
  lv_obj_t *needle_line;
  int32_t needle_length;
};

void set_analog_dial_value(struct analog_dial_t *dial, int32_t value)
{
  lv_scale_set_line_needle_value(
      dial->scale,
      dial->needle_line,
      dial->needle_length,
      value);
}

struct analog_dial_t *create_analog_dial(
    lv_obj_t *parent)
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

  lv_obj_t *needle_line = lv_line_create(scale_line);
  dial->needle_line = needle_line;

  lv_obj_set_style_line_width(needle_line, 1, LV_PART_MAIN);
  lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

  dial->needle_length = d2 + major_tick_length;

  set_analog_dial_value(dial, 53);
  // lv_scale_set_line_needle_value(scale_line, needle_line, d2 + major_tick_length, 53);

  return dial;
}

void free_analog_dial(struct analog_dial_t *dial)
{
  /* deletes recursively delete children too but we will still try to delete
     from bottom up explicitly */
  lv_obj_delete(dial->needle_line);
  lv_obj_delete(dial->scale);
  lv_obj_delete(dial->container);
  free(dial);
}
