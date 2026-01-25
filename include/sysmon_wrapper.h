#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Wrapper for xTaskCreate that automatically registers tasks with SysMon
 *
 * This function provides the same interface as xTaskCreate but automatically
 * calls sysmon_stack_register() to enable percentage-based stack monitoring.
 *
 * @param pvTaskCode Pointer to the task entry function
 * @param pcName A descriptive name for the task (max 16 characters)
 * @param usStackDepth The size of the task stack in WORDS (not bytes!)
 * @param pvParameters Pointer that will be used as the parameter for the task
 * @param uxPriority The priority at which the task should run
 * @param pxCreatedTask Used to pass back a handle by which the created task can be referenced
 *
 * @return pdPASS if the task was successfully created and registered, pdFAIL otherwise
 */
BaseType_t sysmon_xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask);

/**
 * @brief Wrapper for xTaskCreatePinnedToCore that automatically registers tasks with SysMon
 *
 * This function provides the same interface as xTaskCreatePinnedToCore but automatically
 * calls sysmon_stack_register() to enable percentage-based stack monitoring.
 *
 * @param pvTaskCode Pointer to the task entry function
 * @param pcName A descriptive name for the task (max 16 characters)
 * @param usStackDepth The size of the task stack in WORDS (not bytes!)
 * @param pvParameters Pointer that will be used as the parameter for the task
 * @param uxPriority The priority at which the task should run
 * @param pxCreatedTask Used to pass back a handle by which the created task can be referenced
 * @param xCoreID The core on which the task should run
 *
 * @return pdPASS if the task was successfully created and registered, pdFAIL otherwise
 */
BaseType_t sysmon_xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pxCreatedTask,
    const BaseType_t xCoreID);
