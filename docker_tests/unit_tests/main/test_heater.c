#include "unity.h"
#include "heater.h"
#include "mock_esp_log.h" // Include the mock for esp_log

// Tag used in heater.c
static const char *TAG = "HEATER";

void setUp(void) {
    // Initialize all mocks
    Mockmock_esp_log_Init();
}

void tearDown(void) {
    // Verify and deinitialize all mocks
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_heater_init(void) {
    // Expect the ESP_LOGI call during initialization
    esp_log_wrapper_i_CMockExpect(UNITY_LINE_NUM, TAG, "Heater initialized (stub)");
    heater_init();
}

void test_set_heat_power(void) {
    // Test various power levels and expect the corresponding log calls
    esp_log_wrapper_d_CMockExpect(UNITY_LINE_NUM, TAG, "Setting heat power to %u");
    set_heat_power(128);

    esp_log_wrapper_d_CMockExpect(UNITY_LINE_NUM, TAG, "Setting heat power to %u");
    set_heat_power(0);

    esp_log_wrapper_d_CMockExpect(UNITY_LINE_NUM, TAG, "Setting heat power to %u");
    set_heat_power(255);
}
