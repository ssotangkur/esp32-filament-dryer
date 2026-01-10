#ifndef FREERTOS_H
#define FREERTOS_H

// Minimal FreeRTOS types and definitions for testing
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS types
typedef void *SemaphoreHandle_t;
typedef void *QueueHandle_t;
typedef void *TaskHandle_t;
typedef void *TimerHandle_t;

// FreeRTOS constants
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS pdTRUE
#define pdFAIL pdFALSE

#define errQUEUE_EMPTY (-1)
#define errQUEUE_FULL (-2)

// Timeout values
#define portMAX_DELAY 0xFFFFFFFF
#define portTICK_PERIOD_MS 1

// Task priorities
#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 25

#endif // FREERTOS_H
