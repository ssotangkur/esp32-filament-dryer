#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Generic circular buffer structure
 */
typedef struct
{
  void *buffer;            // Pointer to buffer data
  size_t element_size;     // Size of each element in bytes
  size_t buffer_size;      // Maximum number of elements
  size_t head;             // Write index
  size_t count;            // Number of valid elements
  SemaphoreHandle_t mutex; // Thread safety mutex
} circular_buffer_t;

/**
 * @brief Initialize a circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param element_size Size of each element in bytes
 * @param buffer_size Maximum number of elements
 * @return true on success, false on failure
 */
bool circular_buffer_init(circular_buffer_t *cb, size_t element_size, size_t buffer_size);

/**
 * @brief Add an element to the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param data Pointer to data to add
 * @return true on success, false on failure
 */
bool circular_buffer_push(circular_buffer_t *cb, const void *data);

/**
 * @brief Get the most recent element from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param data Pointer to buffer to store the retrieved data
 * @return true if data was retrieved, false if buffer is empty
 */
bool circular_buffer_get_latest(circular_buffer_t *cb, void *data);

/**
 * @brief Get element at specific index from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param index Index to retrieve (0 = oldest, count-1 = newest)
 * @param data Pointer to buffer to store the retrieved data
 * @return true if data was retrieved, false if index is invalid
 */
bool circular_buffer_get_at_index(circular_buffer_t *cb, size_t index, void *data);

/**
 * @brief Get the number of elements in the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @return Number of valid elements
 */
size_t circular_buffer_count(circular_buffer_t *cb);

/**
 * @brief Check if circular buffer is empty
 * @param cb Pointer to circular_buffer_t structure
 * @return true if empty, false otherwise
 */
bool circular_buffer_is_empty(circular_buffer_t *cb);

/**
 * @brief Check if circular buffer is full
 * @param cb Pointer to circular_buffer_t structure
 * @return true if full, false otherwise
 */
bool circular_buffer_is_full(circular_buffer_t *cb);

/**
 * @brief Clear all elements from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 */
void circular_buffer_clear(circular_buffer_t *cb);

/**
 * @brief Deinitialize a circular buffer (free resources)
 * @param cb Pointer to circular_buffer_t structure
 */
void circular_buffer_deinit(circular_buffer_t *cb);

#endif // CIRCULAR_BUFFER_H
