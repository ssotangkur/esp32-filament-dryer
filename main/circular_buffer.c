#include "circular_buffer.h"
#include "esp_heap_caps.h"
#include <string.h>

static bool notify_subscribers(circular_buffer_t *cb, const void *data, size_t count);

/**
 * @brief Initialize a circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param element_size Size of each element in bytes
 * @param buffer_size Maximum number of elements
 * @return true on success, false on failure
 */
bool circular_buffer_init(circular_buffer_t *cb, size_t element_size, size_t buffer_size)
{
  if (cb == NULL || element_size == 0 || buffer_size == 0)
  {
    return false;
  }

  // Allocate buffer in PSRAM for large buffers
  cb->buffer = heap_caps_malloc(buffer_size * element_size, MALLOC_CAP_SPIRAM);
  if (cb->buffer == NULL)
  {
    return false;
  }

  // Initialize buffer to zero
  memset(cb->buffer, 0, buffer_size * element_size);

  cb->element_size = element_size;
  cb->buffer_size = buffer_size;
  cb->head = 0;
  cb->count = 0;
  cb->subscribers = NULL;
  cb->subscriber_count = 0;

  // Create mutex for thread safety
  cb->mutex = xSemaphoreCreateMutex();
  if (cb->mutex == NULL)
  {
    heap_caps_free(cb->buffer);
    cb->buffer = NULL;
    return false;
  }

  return true;
}

/**
 * @brief Add an element to the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param data Pointer to data to add
 * @return true on success, false on failure
 */
bool circular_buffer_push(circular_buffer_t *cb, const void *data)
{
  if (cb == NULL || cb->buffer == NULL || data == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  // Calculate buffer position for new element
  void *dest = (uint8_t *)cb->buffer + (cb->head * cb->element_size);

  // Copy data to buffer
  memcpy(dest, data, cb->element_size);

  // Update buffer state
  cb->head = (cb->head + 1) % cb->buffer_size;

  if (cb->count < cb->buffer_size)
  {
    cb->count++;
  }

  // Notify all subscribers about the new data point
  if (cb->subscriber_count > 0)
  {
    notify_subscribers(cb, dest, 1);
  }

  xSemaphoreGive(cb->mutex);
  return true;
}

/**
 * @brief Get the most recent element from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param data Pointer to buffer to store the retrieved data
 * @return true if data was retrieved, false if buffer is empty
 */
bool circular_buffer_get_latest(circular_buffer_t *cb, void *data)
{
  if (cb == NULL || cb->buffer == NULL || data == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  bool result = false;

  if (cb->count > 0)
  {
    // Get the most recent sample (head - 1, wrapping around)
    size_t latest_index = (cb->head + cb->buffer_size - 1) % cb->buffer_size;
    void *src = (uint8_t *)cb->buffer + (latest_index * cb->element_size);

    memcpy(data, src, cb->element_size);
    result = true;
  }

  xSemaphoreGive(cb->mutex);
  return result;
}

/**
 * @brief Get element at specific index from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @param index Index to retrieve (0 = oldest, count-1 = newest)
 * @param data Pointer to buffer to store the retrieved data
 * @return true if data was retrieved, false if index is invalid
 */
bool circular_buffer_get_at_index(circular_buffer_t *cb, size_t index, void *data)
{
  if (cb == NULL || cb->buffer == NULL || data == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  bool result = false;

  if (index < cb->count)
  {
    // Calculate buffer index (head - count + index, wrapping around)
    size_t buffer_index = (cb->head + cb->buffer_size - cb->count + index) % cb->buffer_size;
    void *src = (uint8_t *)cb->buffer + (buffer_index * cb->element_size);

    memcpy(data, src, cb->element_size);
    result = true;
  }

  xSemaphoreGive(cb->mutex);
  return result;
}

/**
 * @brief Get the number of elements in the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 * @return Number of valid elements
 */
size_t circular_buffer_count(circular_buffer_t *cb)
{
  if (cb == NULL)
  {
    return 0;
  }

  size_t count = 0;

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) == pdTRUE)
  {
    count = cb->count;
    xSemaphoreGive(cb->mutex);
  }

  return count;
}

/**
 * @brief Check if circular buffer is empty
 * @param cb Pointer to circular_buffer_t structure
 * @return true if empty, false otherwise
 */
bool circular_buffer_is_empty(circular_buffer_t *cb)
{
  return circular_buffer_count(cb) == 0;
}

/**
 * @brief Check if circular buffer is full
 * @param cb Pointer to circular_buffer_t structure
 * @return true if full, false otherwise
 */
bool circular_buffer_is_full(circular_buffer_t *cb)
{
  if (cb == NULL)
  {
    return false;
  }

  bool is_full = false;

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) == pdTRUE)
  {
    is_full = (cb->count == cb->buffer_size);
    xSemaphoreGive(cb->mutex);
  }

  return is_full;
}

/**
 * @brief Clear all elements from the circular buffer
 * @param cb Pointer to circular_buffer_t structure
 */
void circular_buffer_clear(circular_buffer_t *cb)
{
  if (cb == NULL || cb->buffer == NULL)
  {
    return;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) == pdTRUE)
  {
    cb->head = 0;
    cb->count = 0;
    xSemaphoreGive(cb->mutex);
  }
}
/**
 * @brief Free a circular buffer (free resources)
 * @param cb Pointer to circular_buffer_t structure
 */
void circular_buffer_free(circular_buffer_t *cb)
{
  if (cb == NULL)
  {
    return;
  }

  if (cb->subscribers != NULL)
  {
    heap_caps_free(cb->subscribers);
    cb->subscribers = NULL;
  }
  cb->subscriber_count = 0;

  if (cb->mutex != NULL)
  {
    vSemaphoreDelete(cb->mutex);
    cb->mutex = NULL;
  }

  if (cb->buffer != NULL)
  {
    heap_caps_free(cb->buffer);
    cb->buffer = NULL;
  }

  cb->element_size = 0;
  cb->buffer_size = 0;
  cb->head = 0;
  cb->count = 0;
}

static bool notify_subscribers(circular_buffer_t *cb, const void *data, size_t count)
{
  for (size_t i = 0; i < cb->subscriber_count; i++)
  {
    if (cb->subscribers[i].callback != NULL)
    {
      cb->subscribers[i].callback(data, count, cb->subscribers[i].user_data);
    }
  }
  return true;
}

bool circular_buffer_subscribe(circular_buffer_t *cb, circular_buffer_subscriber_cb_t callback, void *user_data)
{
  if (cb == NULL || callback == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  circular_buffer_subscriber_t *new_subscribers = heap_caps_malloc(
      (cb->subscriber_count + 1) * sizeof(circular_buffer_subscriber_t),
      MALLOC_CAP_SPIRAM);

  if (new_subscribers == NULL)
  {
    xSemaphoreGive(cb->mutex);
    return false;
  }

  if (cb->subscribers != NULL)
  {
    memcpy(new_subscribers, cb->subscribers, cb->subscriber_count * sizeof(circular_buffer_subscriber_t));
    heap_caps_free(cb->subscribers);
  }

  cb->subscribers = new_subscribers;
  cb->subscribers[cb->subscriber_count].callback = callback;
  cb->subscribers[cb->subscriber_count].user_data = user_data;
  cb->subscriber_count++;

  // Send all historical data to the new subscriber
  if (cb->count > 0)
  {
    // Allocate temporary buffer to hold all data
    void *hist_data = heap_caps_malloc(cb->count * cb->element_size, MALLOC_CAP_SPIRAM);
    if (hist_data != NULL)
    {
      // Copy all elements in order (oldest to newest)
      for (size_t i = 0; i < cb->count; i++)
      {
        size_t buffer_index = (cb->head + cb->buffer_size - cb->count + i) % cb->buffer_size;
        void *src = (uint8_t *)cb->buffer + (buffer_index * cb->element_size);
        void *dest = (uint8_t *)hist_data + (i * cb->element_size);
        memcpy(dest, src, cb->element_size);
      }

      // Call subscriber with all historical data
      callback(hist_data, cb->count, user_data);
      heap_caps_free(hist_data);
    }
  }

  xSemaphoreGive(cb->mutex);
  return true;
}

bool circular_buffer_unsubscribe(circular_buffer_t *cb, void *user_data)
{
  if (cb == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  // Find and remove the subscriber
  for (size_t i = 0; i < cb->subscriber_count; i++)
  {
    if (cb->subscribers[i].user_data == user_data)
    {
      // Shift remaining subscribers
      for (size_t j = i; j < cb->subscriber_count - 1; j++)
      {
        cb->subscribers[j] = cb->subscribers[j + 1];
      }
      cb->subscriber_count--;

      if (cb->subscriber_count == 0)
      {
        heap_caps_free(cb->subscribers);
        cb->subscribers = NULL;
      }
      else
      {
        circular_buffer_subscriber_t *new_subscribers = heap_caps_malloc(
            cb->subscriber_count * sizeof(circular_buffer_subscriber_t),
            MALLOC_CAP_SPIRAM);
        if (new_subscribers != NULL)
        {
          memcpy(new_subscribers, cb->subscribers, cb->subscriber_count * sizeof(circular_buffer_subscriber_t));
          heap_caps_free(cb->subscribers);
          cb->subscribers = new_subscribers;
        }
      }

      xSemaphoreGive(cb->mutex);
      return true;
    }
  }

  xSemaphoreGive(cb->mutex);
  return false;
}

bool circular_buffer_iterate(circular_buffer_t *cb, bool (*callback)(size_t index, const void *data, void *user_data), void *user_data)
{
  if (cb == NULL || callback == NULL)
  {
    return false;
  }

  if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE)
  {
    return false;
  }

  bool result = true;

  for (size_t i = 0; i < cb->count; i++)
  {
    size_t buffer_index = (cb->head + cb->buffer_size - cb->count + i) % cb->buffer_size;
    void *data = (uint8_t *)cb->buffer + (buffer_index * cb->element_size);

    if (!callback(i, data, user_data))
    {
      result = false;
      break;
    }
  }

  xSemaphoreGive(cb->mutex);
  return result;
}
