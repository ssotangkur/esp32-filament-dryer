#include <stdio.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/analog_dial.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "UI";

static lv_obj_t *needle_line;

static void set_needle_line_value(lv_obj_t *obj, int32_t v)
{
    lv_scale_set_line_needle_value(obj, needle_line, 60, v);
}

/* Initialize user interface */
void init_ui(void)
{

    lvgl_port_lock(0);
    ESP_LOGI(TAG, "UI initialization started - creating minimal test objects");

    /* Change the active screen's background color */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);

    lv_obj_set_width(lv_screen_active(), lv_pct(100));
    lv_obj_set_height(lv_screen_active(), lv_pct(100));
    lv_obj_set_layout(lv_screen_active(), LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lv_screen_active(), LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lv_screen_active(), LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Create a single test label */
    lv_obj_t *test_label = lv_label_create(lv_screen_active());
    lv_label_set_text(test_label, "LVGL 9.x");

    /* Create round scale */
    // lv_obj_t *scale_line = lv_scale_create(lv_screen_active());

    // lv_obj_set_size(scale_line, 100, 100);
    // lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_INNER);
    // lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
    // lv_obj_set_style_bg_color(scale_line, lv_color_white(), 0);
    // lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
    // lv_obj_set_style_clip_corner(scale_line, true, 0);
    // lv_obj_align(scale_line, LV_ALIGN_LEFT_MID, LV_PCT(2), 0);

    // lv_scale_set_label_show(scale_line, true);

    // lv_scale_set_total_tick_count(scale_line, 31);
    // lv_scale_set_major_tick_every(scale_line, 5);

    // lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
    // lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
    // lv_scale_set_range(scale_line, 10, 40);

    // lv_scale_set_angle_range(scale_line, 270);
    // lv_scale_set_rotation(scale_line, 135);

    // needle_line = lv_line_create(scale_line);

    // lv_obj_set_style_line_width(needle_line, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
    // lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

    // set_needle_line_value(scale_line, 32);

    struct analog_dial_t *dial = create_analog_dial(lv_screen_active());
    set_analog_dial_value(dial, 75);

    /* Try any of the demos by uncommenting one of the lines below */
    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_benchmark();
    // lv_demo_stress();

    ESP_LOGI(TAG, "UI initialization completed - objects created successfully");
    lvgl_port_unlock();
}