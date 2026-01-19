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

// FPS monitor event callback - LVGL 9.x uses events instead of callbacks
static void fps_monitor_event_cb(lv_event_t *e)
{
       if (fps_monitoring)
       {
              // Get the display object
              lv_display_t *disp = lv_event_get_target(e);

              // In LVGL 9.x, we need to get timing information differently
              // This is a simplified version - you may need to adjust based on available APIs
              fps_frame_count++;

              // Note: LVGL 9.x may have different timing APIs
              // This is a placeholder - you may need to use lv_tick_get() or similar
              uint32_t current_time = lv_tick_get();
              static uint32_t last_time = 0;

              if (last_time != 0)
              {
                     fps_time_sum += (current_time - last_time);
              }
              last_time = current_time;
       }
}

// Reset FPS counters
void fps_monitor_reset(void)
{
       fps_frame_count = 0;
       fps_time_sum = 0;
}

// Setup FPS monitoring event callback for display
void fps_monitor_setup_callback(void *disp_ptr)
{
       lv_display_t *disp = (lv_display_t *)disp_ptr;
       if (disp)
       {
              // In LVGL 9.x, use events instead of direct callback assignment
              lv_display_add_event_cb(disp, fps_monitor_event_cb, LV_EVENT_RENDER_READY, NULL);
       }
}

// Get current FPS value
uint32_t fps_monitor_get_fps(void)
{
       if (fps_frame_count == 0 || fps_time_sum == 0)
       {
              return 0;
       }

       // Calculate FPS: frames / (time_sum / 1000) = frames * 1000 / time_sum
       uint32_t fps = (fps_frame_count * 1000) / fps_time_sum;
       return fps;
}

// Start FPS monitoring
void fps_monitor_start(void)
{
       fps_monitoring = true;
       fps_monitor_reset();
}
