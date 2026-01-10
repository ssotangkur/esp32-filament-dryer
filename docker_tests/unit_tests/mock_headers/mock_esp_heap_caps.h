#ifndef MOCK_ESP_HEAP_CAPS_H
#define MOCK_ESP_HEAP_CAPS_H

#include <stddef.h>
#include <stdint.h>

// Mock ESP heap capabilities
#define MALLOC_CAP_SPIRAM 0x01

// Mock heap functions (will be mocked by CMock)
void *heap_caps_malloc(size_t size, uint32_t caps);
void heap_caps_free(void *ptr);

#endif // MOCK_ESP_HEAP_CAPS_H
