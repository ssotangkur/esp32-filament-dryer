#ifndef SYSMON_WRAPPER_H
#define SYSMON_WRAPPER_H

// Include basic FreeRTOS types
#include "freertos/FreeRTOS.h"

// Task function type
typedef void (*TaskFunction_t)(void *);

// SysMon wrapper function prototypes (will be mocked by CMock)
BaseType_t sysmon_xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask);

BaseType_t sysmon_xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask,
    const BaseType_t xCoreID);

#endif // SYSMON_WRAPPER_H
