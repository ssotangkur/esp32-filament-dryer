#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

static const char *TAG = "PSRAM_TEST";

void run_psram_tests(void)
{
  // Test PSRAM allocation
  size_t psram_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  ESP_LOGI(TAG, "PSRAM free size: %zu bytes", psram_size);

  if (psram_size > 0)
  {
    // Try to allocate a buffer in PSRAM
    void *psram_buffer = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    if (psram_buffer)
    {
      ESP_LOGI(TAG, "Successfully allocated 1KB buffer in PSRAM");
      memset(psram_buffer, 0xAA, 1024);
      ESP_LOGI(TAG, "PSRAM buffer filled with 0xAA pattern");

      // Verify the pattern
      uint8_t *buf = (uint8_t *)psram_buffer;
      if (buf[0] == 0xAA && buf[1023] == 0xAA)
      {
        ESP_LOGI(TAG, "PSRAM read/write test PASSED");
      }
      else
      {
        ESP_LOGE(TAG, "PSRAM read/write test FAILED");
      }

      heap_caps_free(psram_buffer);
    }
    else
    {
      ESP_LOGE(TAG, "Failed to allocate buffer in PSRAM");
    }
  }
  else
  {
    ESP_LOGE(TAG, "No PSRAM detected or available");
  }
}

void run_startup_tests(void)
{

  /* Print chip information */
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
         CONFIG_IDF_TARGET,
         chip_info.cores,
         (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
         (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
         (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d, ", major_rev, minor_rev);
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    printf("Get flash size failed");
    return;
  }

  printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

  // Run PSRAM tests
  run_psram_tests();
}
