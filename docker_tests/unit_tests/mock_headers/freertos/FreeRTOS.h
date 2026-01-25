#pragma once

// Minimal FreeRTOS types and definitions for testing
#include <stdint.h>
#include <stdbool.h>

// FreeRTOS types
typedef void *SemaphoreHandle_t;
typedef void *QueueHandle_t;
typedef void *TaskHandle_t;
typedef void *TimerHandle_t;

// FreeRTOS integer types
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef unsigned int TickType_t;

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

// Time conversion macros
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)((xTimeInMs) * 1000 / portTICK_PERIOD_MS))

// Task function type
typedef void (*TaskFunction_t)(void *);

// Task priorities
#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 25
