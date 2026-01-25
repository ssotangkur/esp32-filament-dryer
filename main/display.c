#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_dma_utils.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "product_pins.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "diagnostic.h"
#include "display.h"
#include "temp.h"
#include "wifi.h"
#include "version.h"

// FPS display variables
static lv_obj_t *fps_label = NULL;
static lv_timer_t *fps_update_timer = NULL;

// Temperature display variables
static lv_obj_t *air_temp_label = NULL;
static lv_obj_t *air_vr_label = NULL; // Air voltage|resistance
static lv_obj_t *heater_temp_label = NULL;
static lv_obj_t *heater_vr_label = NULL; // Heater voltage|resistance
static lv_obj_t *ip_label = NULL;
static lv_timer_t *temp_update_timer = NULL;

// Temperature meter variables
static lv_obj_t *air_meter = NULL;
static lv_obj_t *heater_meter = NULL;

static lv_obj_t *needle_line;

static void set_needle_line_value(lv_obj_t *obj, int32_t v)
{
  lv_scale_set_line_needle_value(obj, needle_line, 60, v);
}

// FPS display update callback
static void fps_update_cb(lv_timer_t *timer)
{
  if (fps_label != NULL)
  {
    uint32_t fps = fps_monitor_get_fps();
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "FPS: %lu", fps);
    lv_label_set_text(fps_label, buf);
  }
}

// Temperature display update callback
static void temp_update_cb(lv_timer_t *timer)
{
  // Update air sensor temperature
  if (air_temp_label != NULL)
  {
    temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
    if (air_sensor != NULL)
    {
      float temp = temp_sensor_get_reading(air_sensor);

      if (temp == -999.0f)
      {
        // No samples available yet
        lv_label_set_text(air_temp_label, "Air: --°C");
        printf("Air temperature: No samples available yet\n");
      }
      else
      {
        char buf[32];

        // Format: Temperature with 1 decimal place
        int temp_int = (int)(temp * 10);
        int temp_whole = temp_int / 10;
        int temp_decimal = temp_int % 10;

        lv_snprintf(buf, sizeof(buf), "Air: %d.%d°C", temp_whole, temp_decimal);

        lv_label_set_text(air_temp_label, buf);
        // printf("Air temperature updated: %.1f°C (display: %s)\n", temp, buf);

        // TODO: Update air temperature scale indicator when LVGL 9.x scale API is fully understood
      }
    }
  }

  // Update air sensor voltage|resistance
  if (air_vr_label != NULL)
  {
    temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
    if (air_sensor != NULL)
    {
      float voltage = temp_sensor_get_voltage(air_sensor);
      float resistance = temp_sensor_get_resistance(air_sensor);

      if (voltage == -999.0f || resistance == -999.0f)
      {
        // No samples available yet
        lv_label_set_text(air_vr_label, "Air: --V|--k");
        printf("Air voltage/resistance: No samples available yet\n");
      }
      else
      {
        char buf[32];

        // Format: Voltage (2 decimals) | Resistance (1 decimal in k)
        int voltage_int = (int)(voltage * 100);
        int voltage_whole = voltage_int / 100;
        int voltage_decimal = voltage_int % 100;

        float resistance_kohm = resistance / 1000.0f;
        int resistance_whole = (int)resistance_kohm;
        int resistance_decimal = (int)((resistance_kohm - resistance_whole) * 10);

        lv_snprintf(buf, sizeof(buf), "Air: %d.%02dV|%d.%dk", voltage_whole, voltage_decimal, resistance_whole, resistance_decimal);

        lv_label_set_text(air_vr_label, buf);
        // printf("Air voltage/resistance updated: %.2fV|%.1fk (display: %s)\n", voltage, resistance_kohm, buf);
      }
    }
  }

  // Update heater sensor temperature
  if (heater_temp_label != NULL)
  {
    temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();
    if (heater_sensor != NULL)
    {
      float temp = temp_sensor_get_reading(heater_sensor);

      if (temp == -999.0f)
      {
        // No samples available yet
        lv_label_set_text(heater_temp_label, "Heat: --°C");
        printf("Heater temperature: No samples available yet\n");
      }
      else
      {
        char buf[32];

        // Format: Temperature with 1 decimal place
        int temp_int = (int)(temp * 10);
        int temp_whole = temp_int / 10;
        int temp_decimal = temp_int % 10;

        lv_snprintf(buf, sizeof(buf), "Heat: %d.%d°C", temp_whole, temp_decimal);

        lv_label_set_text(heater_temp_label, buf);
        // printf("Heater temperature updated: %.1f°C (display: %s)\n", temp, buf);

        // TODO: Update heater temperature scale indicator when LVGL 9.x scale API is fully understood
      }
    }
  }

  // Update heater sensor voltage|resistance
  if (heater_vr_label != NULL)
  {
    temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();
    if (heater_sensor != NULL)
    {
      float voltage = temp_sensor_get_voltage(heater_sensor);
      float resistance = temp_sensor_get_resistance(heater_sensor);

      if (voltage == -999.0f || resistance == -999.0f)
      {
        // No samples available yet
        lv_label_set_text(heater_vr_label, "Heat: --V|--k");
        printf("Heater voltage/resistance: No samples available yet\n");
      }
      else
      {
        char buf[32];

        // Format: Voltage (2 decimals) | Resistance (1 decimal in k)
        int voltage_int = (int)(voltage * 100);
        int voltage_whole = voltage_int / 100;
        int voltage_decimal = voltage_int % 100;

        float resistance_kohm = resistance / 1000.0f;
        int resistance_whole = (int)resistance_kohm;
        int resistance_decimal = (int)((resistance_kohm - resistance_whole) * 10);

        lv_snprintf(buf, sizeof(buf), "Heat: %d.%02dV|%d.%dk", voltage_whole, voltage_decimal, resistance_whole, resistance_decimal);

        lv_label_set_text(heater_vr_label, buf);
        // printf("Heater voltage/resistance updated: %.2fV|%.1fk (display: %s)\n", voltage, resistance_kohm, buf);
      }
    }
  }

  // Update IP address label
  if (ip_label != NULL)
  {
    char ip_buffer[16];
    esp_err_t ret = wifi_get_ip_address(ip_buffer, sizeof(ip_buffer));
    if (ret == ESP_OK && ip_buffer[0] != '\0')
    {
      char buf[32];
      lv_snprintf(buf, sizeof(buf), "IP: %s", ip_buffer);
      lv_label_set_text(ip_label, buf);
      // printf("IP address updated: %s\n", ip_buffer);
    }
    else
    {
      lv_label_set_text(ip_label, "IP: Not connected");
      printf("IP address: Not connected\n");
    }
  }
}

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)

static const char *TAG = "TFT";

typedef struct
{
  uint8_t addr;
  uint8_t param[14];
  uint8_t len;
} lcd_cmd_t;

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1}, // 0x05 for 16bit color RGB565 format
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0x2A, {0X00, 0X23, 0X00, 0XCC}, 4}, // Column Address Set: 35 to 204 (170 pixels, accounting for ST7789V offset)
    {0x2B, {0X00, 0X00, 0X01, 0X3F}, 4}, // Row Address Set: 0 to 319 (320 pixels)
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};

void init_lcd_display(void)
{
  printf("Initializing LCD display...\n");

  // Configure RD pin to high level before initializing
  gpio_config_t rd_gpio_config = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = 1ULL << BOARD_TFT_RD};
  ESP_ERROR_CHECK(gpio_config(&rd_gpio_config));
  gpio_set_level(BOARD_TFT_RD, 1);

  // I80 bus configuration for LilyGo T-Display S3 (8-bit parallel)
  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {
      .dc_gpio_num = BOARD_TFT_DC,
      .wr_gpio_num = BOARD_TFT_WR,
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .data_gpio_nums = {
          BOARD_TFT_DATA0,
          BOARD_TFT_DATA1,
          BOARD_TFT_DATA2,
          BOARD_TFT_DATA3,
          BOARD_TFT_DATA4,
          BOARD_TFT_DATA5,
          BOARD_TFT_DATA6,
          BOARD_TFT_DATA7,
      },
      .bus_width = 8,
      .max_transfer_bytes = AMOLED_HEIGHT * 32 * sizeof(uint16_t), // Full screen capacity
      .psram_trans_align = 64,
      .sram_trans_align = 4};
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

  // LCD panel IO configuration
  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = BOARD_TFT_CS,
      .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
      .trans_queue_depth = 10,
      .on_color_trans_done = NULL, // lvgl_port handles this callback
      .user_ctx = NULL,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .dc_levels = {
          .dc_idle_level = 0,
          .dc_cmd_level = 0,
          .dc_dummy_level = 0,
          .dc_data_level = 1,
      },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

  // LCD panel configuration
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = BOARD_TFT_RST,
      .color_space = ESP_LCD_COLOR_SPACE_RGB,
      .bits_per_pixel = 16,
  };

  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  // Initialize LCD panel
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true)); // Removed rotation
  // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false)); // Default: no mirroring
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 35, 0)); // Add x_gap to shift coordinates right

  // Send initialization commands
  for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++)
  {
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, lcd_st7789v[i].addr, lcd_st7789v[i].param, lcd_st7789v[i].len & 0x7f));
    if (lcd_st7789v[i].len & 0x80)
      vTaskDelay(pdMS_TO_TICKS(120));
  }

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // Turn on LCD backlight
  gpio_config_t bk_gpio_config = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = 1ULL << BOARD_TFT_BL};
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
  gpio_set_level(BOARD_TFT_BL, 1);

  printf("LCD display initialized successfully!\n");
}

/* Initialize LVGL with esp_lvgl_port */
static void init_lvgl_display(void)
{
  printf("Initializing LVGL with esp_lvgl_port...\n");

  /* Initialize LVGL library (required for LVGL 9.x) */
  // lv_init();

  /* Set up LVGL tick interface (required for LVGL 9.x) */
  // lv_tick_set_cb((lv_tick_get_cb_t)esp_timer_get_time);

  /* LVGL port initialization */
  static lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_cfg.task_priority = 4;    // Set task priority
  lvgl_cfg.task_stack = 6144;    // Set task stack size
  lvgl_cfg.task_affinity = 1;    // Allow any core affinity
  lvgl_cfg.timer_period_ms = 16; // 16ms timer period (~60Hz) for reduced CPU usage

  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  // Workaround for LVGL 9.x + ESP32-S3 crash: Add delay after lvgl_port_task creation
  // https://github.com/espressif/esp-bsp/issues/475
  vTaskDelay(pdMS_TO_TICKS(10));

  /* Add display to LVGL */
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = AMOLED_WIDTH * 32, // Full screen buffer for LVGL 9.x compatibility
      .double_buffer = false,
      .hres = AMOLED_WIDTH,
      .vres = AMOLED_HEIGHT,
      .monochrome = false,
      .color_format = LV_COLOR_FORMAT_RGB565,
      .rotation = {
          .swap_xy = false,
          .mirror_x = false,
          .mirror_y = false,
      },
      .flags = {
          .buff_dma = false,
          .buff_spiram = false,
          .sw_rotate = false,
          .full_refresh = false,
          .swap_bytes = true,
      }};

  lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
  if (disp == NULL)
  {
    ESP_LOGE(TAG, "Failed to add display to LVGL");
    return;
  }

  // Set as default display (required in LVGL 9.x)
  lv_display_set_default(disp);

  // Set up FPS monitoring callback on the display driver
  // fps_monitor_setup_callback(disp);

  printf("LVGL initialized successfully!\n");
}

/* Initialize display (LCD + LVGL) */
void init_display(void)
{
  init_lcd_display();
  init_lvgl_display();
}

/* Simple LVGL demo */
void lvgl_demo(void)
{

  lvgl_port_lock(0);

  // Create only a single simple label to test basic functionality
  ESP_LOGI(TAG, "LVGL demo started - creating minimal test object");

  /*Change the active screen's background color*/
  lv_obj_set_style_bg_color(lv_screen_active(), lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);

  lv_obj_set_layout(lv_screen_active(), LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(lv_screen_active(), LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(lv_screen_active(), LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

  /* Create a single test label */
  lv_obj_t *test_label = lv_label_create(lv_screen_active());
  lv_label_set_text(test_label, "LVGL 9.x OK");

  /* Create round scale */
  lv_obj_t *scale_line = lv_scale_create(lv_screen_active());

  lv_obj_set_size(scale_line, 100, 100);
  lv_scale_set_mode(scale_line, LV_SCALE_MODE_ROUND_INNER);
  lv_obj_set_style_bg_opa(scale_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(scale_line, lv_color_white(), 0);
  lv_obj_set_style_radius(scale_line, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(scale_line, true, 0);
  lv_obj_align(scale_line, LV_ALIGN_LEFT_MID, LV_PCT(2), 0);

  lv_scale_set_label_show(scale_line, true);

  lv_scale_set_total_tick_count(scale_line, 31);
  lv_scale_set_major_tick_every(scale_line, 5);

  lv_obj_set_style_length(scale_line, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(scale_line, 10, LV_PART_INDICATOR);
  lv_scale_set_range(scale_line, 10, 40);

  lv_scale_set_angle_range(scale_line, 270);
  lv_scale_set_rotation(scale_line, 135);

  needle_line = lv_line_create(scale_line);

  lv_obj_set_style_line_width(needle_line, 3, LV_PART_MAIN);
  lv_obj_set_style_line_color(needle_line, lv_palette_darken(LV_PALETTE_RED, 3), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);

  set_needle_line_value(scale_line, 32);

  /* Try any of the demos by uncommenting one of the lines below */
  // lv_demo_widgets();
  // lv_demo_keypad_encoder();
  // lv_demo_benchmark();
  // lv_demo_stress();

  ESP_LOGI(TAG, "LVGL demo completed - object created successfully");
  lvgl_port_unlock();
}
