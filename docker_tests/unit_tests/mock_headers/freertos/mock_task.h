#ifndef FREERTOS_TASK_H
#define FREERTOS_TASK_H

// Include basic FreeRTOS types
#include "FreeRTOS.h"

// Task function type
typedef void (*TaskFunction_t)(void *);

// Task creation functions (will be mocked by CMock)
// These match the ESP-IDF FreeRTOS task API
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask);

void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskDelay(const TickType_t xTicksToDelay);

// Task utilities
TaskHandle_t xTaskGetCurrentTaskHandle(void);
TickType_t xTaskGetTickCount(void);

// Task priority definitions
#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 25

#endif // FREERTOS_TASK_H
