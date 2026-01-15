#include "heater.h"
#include "esp_log_wrapper.h"

static const char *TAG = "HEATER";

void heater_init(void) {
    // This is where hardware initialization for the heater would go.
    // For example, configuring a GPIO pin or setting up a PWM channel.
    esp_log_wrapper_i(TAG, "Heater initialized (stub)");
}

void set_heat_power(uint8_t power) {
    // This is where the code to set the heater's power level would go.
    // For example, writing to a PWM duty cycle register or setting a GPIO high/low.
    esp_log_wrapper_d(TAG, "Setting heat power to %u", power);
}
