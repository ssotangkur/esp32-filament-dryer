#include <stdio.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/analog_dial.h"
#include "ui/subjects.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "UI";

/* Initialize user interface */
void init_ui(void)
{
    lvgl_port_lock(0);
    ESP_LOGI(TAG, "UI initialization started - creating minimal test objects");

    /* Initialize all subjects before creating UI widgets */
    subjects_init();

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

    /* Create analog dial - bound to heater temperature subject
     * Target: 45°C with 10°C range (40-50°C is green, >50°C is red)
     * Scale range: 0-120°C */
    struct analog_dial_t *dial = create_analog_dial(
        lv_screen_active(),
        &g_subject_heater_temp,
        100.0f,  /* target_value */
        10.0f,   /* target_range */
        0.0f,    /* min_value */
        120.0f); /* max_value */

    /* Try any of the demos by uncommenting one of the lines below */
    // lv_demo_widgets();
    // lv_demo_keypad_encoder();
    // lv_demo_benchmark();
    // lv_demo_stress();

    ESP_LOGI(TAG, "UI initialization completed - objects created successfully");
    lvgl_port_unlock();
}
