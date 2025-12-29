#include "esp_system.h"
#include "esp_heap_caps.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// FPS monitoring variables
static uint32_t fps_frame_count = 0;
static uint32_t fps_time_sum = 0;
static bool fps_monitoring = false;

/* Print memory usage information */
void print_memory_info(void)
{
       // Total heap (SRAM + PSRAM)
       size_t free_heap_total = esp_get_free_heap_size();
       size_t total_heap_total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
       size_t used_heap_total = total_heap_total - free_heap_total;
       float percentage_total = (float)used_heap_total / total_heap_total * 100.0f;

       // Internal SRAM only
       size_t free_heap_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
       size_t total_heap_sram = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
       size_t used_heap_sram = total_heap_sram - free_heap_sram;
       float percentage_sram = (float)used_heap_sram / total_heap_sram * 100.0f;

       printf("Total Heap (SRAM+PSRAM): Used %u bytes, Free %u bytes, Total %u bytes, Used %.2f%%\n",
              (unsigned int)used_heap_total, (unsigned int)free_heap_total, (unsigned int)total_heap_total, percentage_total);

       printf("Internal SRAM: Used %u bytes, Free %u bytes, Total %u bytes, Used %.2f%%\n",
              (unsigned int)used_heap_sram, (unsigned int)free_heap_sram, (unsigned int)total_heap_sram, percentage_sram);
}

/* Print FPS information using our custom FPS monitoring */
void print_fps_info(void)
{
       uint32_t fps = fps_monitor_get_fps();
       printf("Current FPS: %lu\n", fps);
}

/* FPS monitoring functions */

// FPS monitor callback - based on LVGL benchmark implementation
static void fps_monitor_cb(lv_disp_drv_t *drv, uint32_t time, uint32_t px)
{
       if (fps_monitoring)
       {
              fps_frame_count++;
              fps_time_sum += time;
       }
}

// Start FPS monitoring
void fps_monitor_start(void)
{
       fps_frame_count = 0;
       fps_time_sum = 0;
       fps_monitoring = true;
}

// Stop FPS monitoring
void fps_monitor_stop(void)
{
       fps_monitoring = false;
}

// Get current FPS - based on LVGL benchmark calculation: fps = (1000 * frames) / total_ms
uint32_t fps_monitor_get_fps(void)
{
       if (fps_time_sum == 0)
       {
              return 0; // Avoid division by zero
       }
       return (1000UL * fps_frame_count) / fps_time_sum;
}

// Reset FPS counters
void fps_monitor_reset(void)
{
       fps_frame_count = 0;
       fps_time_sum = 0;
}

// Setup FPS monitoring callback for display driver
void fps_monitor_setup_callback(void *disp_ptr)
{
       lv_display_t *disp = (lv_display_t *)disp_ptr;
       if (disp && disp->driver)
       {
              disp->driver->monitor_cb = fps_monitor_cb;
       }
}
