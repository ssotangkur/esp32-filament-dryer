#include "heater.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "product_pins.h" // For BOARD_HEATER_GPIO

static const char *TAG = "HEATER";

void heater_init(void)
{
    // Configure heater GPIO as output
    gpio_reset_pin(BOARD_HEATER_GPIO);
    gpio_set_direction(BOARD_HEATER_GPIO, GPIO_MODE_OUTPUT);

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT, // 0 - 255
        .freq_hz = 5000,                     // Set output frequency at 5 kHz
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BOARD_HEATER_GPIO,
        .duty = 0, // Set duty cycle to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_LOGI(TAG, "Heater initialized using GPIO %d", BOARD_HEATER_GPIO);
}

void set_heat_power(uint8_t power)
{
    // Set duty cycle and update atomically for thread safety
    ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, power, 0));
    ESP_LOGD(TAG, "Setting heat power to %u", power);
}
