/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "startup_tests.h"
#include "display.h"
#include "diagnostic.h"
#include "wifi.h"
#include "ota.h"
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

    // Initialize OTA functionality
    ESP_ERROR_CHECK(ota_init());

    // Start automatic periodic OTA checking (checks every 5 seconds)
    ESP_ERROR_CHECK(ota_start_auto_check());

    // Initialize system monitor after WiFi is connected
#ifdef CONFIG_ENABLE_SYSMON
    ESP_ERROR_CHECK(sysmon_init());
#endif

    // Initialize LCD display
    init_display();

    // Start FPS monitoring
    fps_monitor_start();

    // Create LVGL demo
    // lv_demo_widgets();

    lvgl_demo();

    // Keep the app running
    while (1)
    {
        // Reset watchdog timer regularly
        vTaskDelay(pdMS_TO_TICKS(100));

        // Print diagnostic info less frequently to avoid watchdog issues
        static int counter = 0;
        counter++;
        if (counter >= 30)
        { // Every 3 seconds (30 * 100ms)
            print_memory_info();
            print_fps_info();
            printf("LVGL demo running...\n");
            counter = 0;
        }
    }
}
