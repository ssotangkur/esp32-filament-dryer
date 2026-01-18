/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "startup_tests.h"
#include "display.h"
#include "diagnostic.h"
#include "wifi.h"
#include "ota.h"
#include "web_server.h"
#include "temp.h"
#include <sysmon.h>
#include <sysmon_stack.h>

void app_main(void)
{
    printf("Hello world!\n");

    run_startup_tests();

    // Initialize WiFi
    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(wifi_connect());
    ESP_ERROR_CHECK(wifi_wait_for_connection());

    // Initialize web server
    ESP_ERROR_CHECK(web_server_init());

    // Initialize OTA functionality
    ESP_ERROR_CHECK(ota_init());

    // Perform one-time OTA check at boot
    ESP_ERROR_CHECK(ota_check_at_boot());

    // Initialize system monitor after WiFi is connected (without HTTP server)
#ifdef CONFIG_ENABLE_SYSMON
    ESP_ERROR_CHECK(sysmon_init());
#endif

    // Initialize LCD display
    init_display();

    // Initialize temperature sensors
    temp_sensor_init();

    // Start web server
    ESP_ERROR_CHECK(web_server_start());

    // Start FPS monitoring
    fps_monitor_start();

    lvgl_demo();
}
