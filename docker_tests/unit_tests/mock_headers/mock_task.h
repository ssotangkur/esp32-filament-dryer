#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "freertos/FreeRTOS.h"

// FreeRTOS task function prototypes for mocking
void vTaskDelay(const TickType_t xTicksToDelay);
TickType_t xTaskGetTickCount(void);
void vTaskDelete(TaskHandle_t xTaskToDelete);

#endif // MOCK_FREERTOS_TASK_H
