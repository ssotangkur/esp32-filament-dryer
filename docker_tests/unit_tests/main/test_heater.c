#include "unity.h"
#include "heater.h"
#include "Mockmock_esp_log.h"

// Tag used in heater.c
static const char *TAG = "HEATER";

void test_heater_init(void) {
    Mockmock_esp_log_Init();
    esp_log_wrapper_i_ExpectAnyArgs(); // Simplified for now
    heater_init();
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_set_heat_power(void) {
    Mockmock_esp_log_Init();
    esp_log_wrapper_d_ExpectAnyArgs();
    set_heat_power(128);

    esp_log_wrapper_d_ExpectAnyArgs();
    set_heat_power(0);

    esp_log_wrapper_d_ExpectAnyArgs();
    set_heat_power(255);
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}
