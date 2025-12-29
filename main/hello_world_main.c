/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "startup_tests.h"
#include "display.h"
#include "diagnostic.h"
#include "wifi.h"
#include "ota.h"
#include <sysmon.h>
#include <sysmon_stack.h>
#include "esp_log.h"

static const char *TAG = "MAIN";

// Periodic OTA checking task
static void ota_check_task(void *pvParameter)
{
    ESP_LOGI(TAG, "OTA check task started - checking for updates every 5 seconds");

    while (1)
    {
        // Check for OTA updates
        if (ota_check_for_update(OTA_URL))
        {
            ESP_LOGI(TAG, "New firmware version available! Starting update...");
            ota_update_from_url(OTA_URL);

            // Wait a bit before checking again after an update attempt
            vTaskDelay(pdMS_TO_TICKS(30000)); // 30 seconds
        }
        else
        {
            ESP_LOGD(TAG, "Firmware is up to date");
        }

        // Wait 5 seconds before next check
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

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

    // Start periodic OTA checking task (checks every 5 seconds)
    xTaskCreate(&ota_check_task, "ota_check_task", 4096, NULL, 5, NULL);

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
        vTaskDelay(pdMS_TO_TICKS(1000));
        print_memory_info();
        print_fps_info();
        printf("LVGL demo running...\n");
    }
}
