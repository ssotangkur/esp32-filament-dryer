#pragma once

// Include basic FreeRTOS types
#include "FreeRTOS.h"

// Semaphore function prototypes (will be mocked by CMock)
// These match the ESP-IDF FreeRTOS semaphore API
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateBinary(void);
SemaphoreHandle_t xSemaphoreCreateCounting(uint32_t uxMaxCount, uint32_t uxInitialCount);

int xSemaphoreTake(SemaphoreHandle_t xSemaphore, uint32_t xTicksToWait);
int xSemaphoreGive(SemaphoreHandle_t xSemaphore);
int xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, signed long *pxHigherPriorityTaskWoken);

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);

// Queue functions (for compatibility)
QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize);
int xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, uint32_t xTicksToWait);
int xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait);
void vQueueDelete(QueueHandle_t xQueue);
