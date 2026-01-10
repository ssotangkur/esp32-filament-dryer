#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Simplified circular buffer for unit testing (no FreeRTOS dependencies)

typedef struct
{
  void *buffer;        // Pointer to buffer data
  size_t element_size; // Size of each element in bytes
  size_t buffer_size;  // Maximum number of elements
  size_t head;         // Write index
  size_t count;        // Number of valid elements
} circular_buffer_t;

// Initialize a circular buffer
bool circular_buffer_init(circular_buffer_t *cb, size_t element_size, size_t buffer_size)
{
  if (!cb || element_size == 0 || buffer_size == 0)
  {
    return false;
  }

  cb->buffer = malloc(element_size * buffer_size);
  if (!cb->buffer)
  {
    return false;
  }

  cb->element_size = element_size;
  cb->buffer_size = buffer_size;
  cb->head = 0;
  cb->count = 0;

  return true;
}

// Add an element to the circular buffer
bool circular_buffer_push(circular_buffer_t *cb, const void *data)
{
  if (!cb || !data || !cb->buffer)
  {
    return false;
  }

  size_t index = (cb->head + cb->count) % cb->buffer_size;
  memcpy((char *)cb->buffer + (index * cb->element_size), data, cb->element_size);

  if (cb->count < cb->buffer_size)
  {
    cb->count++;
  }
  else
  {
    // Overwrite oldest element
    cb->head = (cb->head + 1) % cb->buffer_size;
  }

  return true;
}

// Get the most recent element from the circular buffer
bool circular_buffer_get_latest(circular_buffer_t *cb, void *data)
{
  if (!cb || !data || !cb->buffer || cb->count == 0)
  {
    return false;
  }

  size_t latest_index = (cb->head + cb->count - 1) % cb->buffer_size;
  memcpy(data, (char *)cb->buffer + (latest_index * cb->element_size), cb->element_size);
  return true;
}

// Get element at specific index from the circular buffer
bool circular_buffer_get_at_index(circular_buffer_t *cb, size_t index, void *data)
{
  if (!cb || !data || !cb->buffer || index >= cb->count)
  {
    return false;
  }

  size_t actual_index = (cb->head + index) % cb->buffer_size;
  memcpy(data, (char *)cb->buffer + (actual_index * cb->element_size), cb->element_size);
  return true;
}

// Get the number of elements in the circular buffer
size_t circular_buffer_count(circular_buffer_t *cb)
{
  return cb ? cb->count : 0;
}

// Check if circular buffer is empty
bool circular_buffer_is_empty(circular_buffer_t *cb)
{
  return cb ? (cb->count == 0) : true;
}

// Check if circular buffer is full
bool circular_buffer_is_full(circular_buffer_t *cb)
{
  return cb ? (cb->count == cb->buffer_size) : false;
}

// Clear all elements from the circular buffer
void circular_buffer_clear(circular_buffer_t *cb)
{
  if (cb)
  {
    cb->head = 0;
    cb->count = 0;
  }
}

// Free a circular buffer (free resources)
void circular_buffer_free(circular_buffer_t *cb)
{
  if (cb && cb->buffer)
  {
    free(cb->buffer);
    cb->buffer = NULL;
    cb->count = 0;
    cb->head = 0;
  }
}
