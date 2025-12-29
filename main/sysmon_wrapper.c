// SysMon component headers
#include <sysmon.h>
#include <sysmon_stack.h>

// Local sysmon wrapper functions
#include "sysmon_wrapper.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "sysmon_task";

BaseType_t sysmon_xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask)
{
  // Create the task using standard FreeRTOS function
  BaseType_t result = xTaskCreate(pvTaskCode, pcName, usStackDepth,
                                  pvParameters, uxPriority, pxCreatedTask);

#ifdef CONFIG_ENABLE_SYSMON
  // If task creation succeeded, register it with SysMon for stack monitoring
  if (result == pdPASS && pxCreatedTask != NULL && *pxCreatedTask != NULL)
  {
    sysmon_stack_register(*pxCreatedTask, usStackDepth);
    ESP_LOGD(TAG, "Task '%s' registered with SysMon stack monitoring",
             pcName ? pcName : "unnamed");
  }
#endif

  return result;
}

BaseType_t sysmon_xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask,
    const BaseType_t xCoreID)
{
  // Create the task using standard FreeRTOS function
  BaseType_t result = xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth,
                                              pvParameters, uxPriority, pxCreatedTask, xCoreID);

#ifdef CONFIG_ENABLE_SYSMON
  // If task creation succeeded, register it with SysMon for stack monitoring
  if (result == pdPASS && pxCreatedTask != NULL && *pxCreatedTask != NULL)
  {
    sysmon_stack_register(*pxCreatedTask, usStackDepth);
    ESP_LOGD(TAG, "Task '%s' registered with SysMon stack monitoring",
             pcName ? pcName : "unnamed");
  }
#endif

  return result;
}
