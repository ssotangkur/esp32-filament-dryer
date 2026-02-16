# Multi-Granularity Time Data Viewing - Implementation Plan

## Overview

Implement browser-side data collection, storage, aggregation, and visualization for temperature data from the ESP32. The ESP32 circular buffer will push data to WebSocket subscribers, and the browser will aggregate and display at different time granularities.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32                                    │
│  ┌──────────────────┐    ┌──────────────────────────────────┐│
│  │  temp.c          │    │  web_websocket.c                 ││
│  │  ─────────────   │    │  ────────────────                 ││
│  │  Circular       │    │  Session context:                 ││
│  │  buffers in     │────┼──  - Subscribes to buffers        ││
│  │  PSRAM          │    │  - Receives historical + new     ││
│  │                 │    │  - Copies to session PSRAM        ││
│  │  temp_buffer_1  │    │  - Enqueues for WebSocket send    ││
│  │  (air)          │    │                                  ││
│  │                 │    │                                  ││
│  │  temp_buffer_2  │    │                                  ││
│  │  (heater)       │    │                                  ││
│  └──────────────────┘    └──────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ WebSocket
┌─────────────────────────────────────────────────────────────────┐
│                      Browser (React)                            │
│  ┌──────────────────┐    ┌──────────────────────────────────┐│
│  │  App.jsx         │    │  useAggregatedData.js            ││
│  │  ─────────       │    │  ──────────────────────           ││
│  │  - Store all    │───▶│  - aggregateBySecond()           ││
│  │    data points  │    │  - aggregateByMinute()           ││
│  │  - Granularity  │    │  - aggregateBy10Min()             ││
│  │    selector     │    │  - aggregateByHour()             ││
│  └──────────────────┘    └──────────────────────────────────┘│
                                    │
                                    ▼
                            ┌──────────────┐
                            │  LineChart   │
                            │  (MUI X)     │
                            └──────────────┘
```

## Data Flow

1. **ESP32 circular buffer** stores temperature samples in PSRAM
2. **WebSocket session** subscribes to circular buffer on connection
3. **Circular buffer** immediately sends all historical data via callback (using iterate)
4. **WebSocket session** receives raw data in chunks (100 readings each), stores in linked list in PSRAM
5. **Subscriber callback** gives semaphore to wake send task
6. **Send task** wakes on semaphore, dequeues all pending messages, serializes to JSON, sends to client
7. **Send task** blocks on semaphore when queue empty (efficient sleep)
8. **On new temperature reading**, circular buffer pushes to all subscribers, callbacks give semaphores to wake send task
9. **Browser** receives data, stores all points, aggregates for display

---

## ESP32 Changes

### 1. Increase Circular Buffer Size

- [ ] **File:** `include/temp.h`
- [ ] **Change:** Set `TEMP_BUFFER_SIZE` to 7200 (~2 hours at 1 sample/sec)
- [ ] **Alternative values:**
  - 3600 = 1 hour
  - 7200 = 2 hours (recommended)
  - 14400 = 4 hours

### 2. Add Circular Buffer Subscriber System

- [ ] **File:** `include/circular_buffer.h`
- [ ] **Changes:**
  - [ ] Add subscriber callback type:
    ```c
    typedef void (*circular_buffer_subscriber_cb_t)(const void *data, size_t count, void *user_data);
    ```
  - [ ] Add subscriber struct:
    ```c
    typedef struct {
      circular_buffer_subscriber_cb_t callback;
      void *user_data;
    } circular_buffer_subscriber_t;
    ```
  - [ ] Add to `circular_buffer_t`:
    ```c
    circular_buffer_subscriber_t *subscribers;
    size_t subscriber_count;
    ```
  - [ ] Add function: `bool circular_buffer_subscribe(circular_buffer_t *cb, circular_buffer_subscriber_cb_t callback, void *user_data);`
  - [ ] Add function: `bool circular_buffer_unsubscribe(circular_buffer_t *cb, void *user_data);`
  - [ ] Add function: `bool circular_buffer_iterate(circular_buffer_t *cb, bool (*callback)(size_t index, const void *data, void *user_data), void *user_data);`

- [ ] **File:** `main/circular_buffer.c`
- [ ] **Changes:**
  - [ ] Implement `circular_buffer_subscribe()`:
    - [ ] Allocate memory for subscriber
    - [ ] Add to subscriber list
    - [ ] **Use `circular_buffer_iterate()` to iterate through ALL buffer contents**
    - [ ] Call subscriber callback with all historical data
    - [ ] After callback returns, release any temporary memory
  - [ ] Implement `circular_buffer_unsubscribe()`
  - [ ] Implement `circular_buffer_iterate()`:
    - [ ] Takes callback function to process each element
    - [ ] Acquires mutex, iterates through valid elements (0 to count-1)
    - [ ] Calls callback for each element with index and data
    - [ ] Returns false if any callback returns false (early exit), true otherwise
  - [ ] Modify `circular_buffer_push()`:
    - [ ] After adding new element, notify ALL subscribers with the new data point
  - [ ] **Add unit tests for subscriber system:**
    - [ ] **File:** `docker_tests/unit_tests/main/test_circular_buffer.c`
    - [ ] Add test: `test_circular_buffer_subscribe()` - verify callback is invoked with all current buffer contents
    - [ ] Add test: `test_circular_buffer_iterate()` - verify iteration function works correctly
    - [ ] Add test: `test_circular_buffer_multiple_subscribers()` - verify multiple subscribers receive data
    - [ ] Add test: `test_circular_buffer_unsubscribe()` - verify subscriber removal works
    - [ ] Add test: `test_circular_buffer_push_notifies_subscribers()` - verify new data pushes to subscribers
    - [ ] Add test: `test_circular_buffer_subscriber_with_null_callback()` - verify NULL callback is handled
    - [ ] Run tests: `.\run_unit_tests.bat`

### 3. Add Temperature Buffer Accessors

- [ ] **File:** `include/temp.h`
- [ ] **Changes:**
  - [ ] Add: `circular_buffer_t* temp_sensor_get_air_buffer(void);`
  - [ ] Add: `circular_buffer_t* temp_sensor_get_heater_buffer(void);`

- [ ] **File:** `main/temp.c`
- [ ] **Changes:**
  - [ ] Implement `temp_sensor_get_air_buffer()` - return `&temp_buffer_1`
  - [ ] Implement `temp_sensor_get_heater_buffer()` - return `&temp_buffer_2`

### 4. Create Linked List Module for Pending Messages

- [ ] **File:** `include/linked_list.h` (new file)
- [ ] **Changes:**
  - [ ] Add node structure:
    ```c
    typedef struct linked_list_node {
        void *data;
        size_t size;
        struct linked_list_node *next;
    } linked_list_node_t;
    ```
  - [ ] Add list structure:
    ```c
    typedef struct {
        linked_list_node_t *head;
        linked_list_node_t *tail;
        size_t count;
        size_t max_size;
        SemaphoreHandle_t mutex;
    } linked_list_t;
    ```
  - [ ] Add chunk size constant:
    ```c
    #define LINKED_LIST_CHUNK_SIZE 100
    ```
  - [ ] Add function declarations:
    - [ ] `bool linked_list_create(linked_list_t *list, size_t max_size);`
    - [ ] `bool linked_list_enqueue(linked_list_t *list, const void *data, size_t size);`
    - [ ] `bool linked_list_dequeue(linked_list_t *list, void *data, size_t *size);`
    - [ ] `void linked_list_destroy(linked_list_t *list);`
    - [ ] `size_t linked_list_count(linked_list_t *list);`

- [ ] **File:** `main/linked_list.c` (new file)
- [ ] **Changes:**
  - [ ] Add `#include "esp_heap_caps.h"`
  - [ ] Add `#include "freertos/FreeRTOS.h"`
  - [ ] Add `#include "freertos/semphr.h"`
  - [ ] Implement `linked_list_create()`:
    - [ ] Initialize head/tail to NULL
    - [ ] Set count to 0
    - [ ] Take max_size as parameter
    - [ ] Create mutex: `xSemaphoreCreateMutex()`
    - [ ] All allocations use PSRAM: `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`
  - [ ] Implement `linked_list_enqueue()`:
    - [ ] Use mutex for thread safety
    - [ ] If count >= max_size: log warning, return false (do NOT drop old)
    - [ ] Allocate node in PSRAM
    - [ ] Copy data to node (caller provides data)
    - [ ] Append to tail, update head if empty
    - [ ] Increment count
  - [ ] Implement `linked_list_dequeue()`:
    - [ ] Use mutex for thread safety
    - [ ] If count == 0: return false
    - [ ] Copy data to caller's buffer
    - [ ] Copy size to caller's size pointer
    - [ ] Remove head node, update head to head->next
    - [ ] If head == NULL, set tail = NULL
    - [ ] Free node memory
    - [ ] Decrement count
  - [ ] Implement `linked_list_destroy()`:
    - [ ] Clear all nodes (dequeue until empty)
    - [ ] Delete mutex
  - [ ] **Add unit tests for linked list:**
    - [ ] **File:** `docker_tests/unit_tests/main/test_linked_list.c` (new file)
    - [ ] Follow pattern from `test_circular_buffer.c`:
      - Include real implementation: `#include "../../main/linked_list.c"`
      - Use CMock mocks for `esp_heap_caps.h` and `freertos/semphr.h`
    - [ ] Add test: `test_linked_list_create()` - verify initialization with max_size
    - [ ] Add test: `test_linked_list_create_failure()` - verify NULL list, zero max_size handling
    - [ ] Add test: `test_linked_list_enqueue_dequeue()` - verify basic enqueue/dequeue operations
    - [ ] Add test: `test_linked_list_multiple_items()` - verify FIFO ordering
    - [ ] Add test: `test_linked_list_full()` - verify enqueue fails when max_size reached
    - [ ] Add test: `test_linked_list_empty_dequeue()` - verify dequeue fails on empty list
    - [ ] Add test: `test_linked_list_destroy()` - verify cleanup frees all nodes
    - [ ] Add test: `test_linked_list_count()` - verify count tracking
    - [ ] Add test: `test_linked_list_null_parameters()` - verify NULL handling
    - [ ] Add test file to `docker_tests/unit_tests/CMakeLists.txt`
    - [ ] Run tests: `.\run_unit_tests.bat`

### 5. Update WebSocket Handler to Use Circular Buffer Subscribers

- [ ] **File:** `main/web/web_websocket.c`
- [ ] **Changes:**
  - [ ] **Add includes:**
    - [ ] Add: `#include "linked_list.h"`
  - [ ] **Remove LVGL subject includes and callbacks:**
    - [ ] Remove: `#include "ui/subjects.h"`
    - [ ] Remove: `air_temp_subject_callback()`
    - [ ] Remove: `heater_temp_subject_callback()`
    - [ ] Remove: `ws_clients_init()` or modify to not subscribe to subjects
  - [ ] **Add circular buffer subscriber callbacks:**
    - [ ] Add: `air_buffer_callback(const void *data, size_t count, void *user_data)`
    - [ ] Add: `heater_buffer_callback(const void *data, size_t count, void *user_data)`
    - [ ] Callback receives raw `temp_sample_t* data` + `count`
    - [ ] Callback chunks data into groups of `LINKED_LIST_CHUNK_SIZE` (100 readings per chunk)
    - [ ] Each chunk: allocate node in PSRAM, copy raw data, enqueue to session's linked list
    - [ ] **Serialization to JSON happens AFTER dequeue** (right before sending), not in callback
  - [ ] **Modify session context (`ws_session_ctx`):**
    - [ ] Add: `linked_list_t pending_msgs` member
    - [ ] Add: `SemaphoreHandle_t send_sem` - binary semaphore to wake send task
    - [ ] Add: `TaskHandle_t send_task_handle` - handle for the send task
  - [ ] **Modify session creation (`get_or_create_session_context`):**
    - [ ] Call `linked_list_create(&sess->pending_msgs, TEMP_BUFFER_SIZE + 100)`
    - [ ] Create send semaphore: `xSemaphoreCreateBinary()` or `xSemaphoreCreateCounting(0, 1)`
    - [ ] Start send task: `xTaskCreate(ws_send_task, "ws_send", stack_size, sess, priority, &sess->send_task_handle)`
    - [ ] Subscribe to air buffer: `circular_buffer_subscribe(temp_sensor_get_air_buffer(), air_buffer_callback, sess);`
    - [ ] Subscribe to heater buffer: `circular_buffer_subscribe(temp_sensor_get_heater_buffer(), heater_buffer_callback, sess);`
  - [ ] **Modify subscriber callback to wake send task:**
    - [ ] After enqueue to linked list, call `xSemaphoreGive(sess->send_sem)` to wake send task
  - [ ] **Add send task implementation (`ws_send_task`):**
    - [ ] Task enters loop: `while (sess->running)`
    - [ ] At start of loop: `xSemaphoreTake(send_sem, portMAX_DELAY)` - blocks until data available
    - [ ] Dequeue all available messages from linked list (dequeue in loop until returns false)
    - [ ] For each dequeued message:
      - [ ] Serialize raw data to JSON
      - [ ] Send via WebSocket: `httpd_ws_send_frame_async(server, sess->sockfd, &ws_resp)`
      - [ ] Free serialized JSON buffer after sending
    - [ ] After processing all messages, loop back to wait on semaphore
    - [ ] If queue empty, semaphore will block again (efficient sleep)
  - [ ] **Modify session cleanup (`cleanup_session_context`):**
    - [ ] Set sess->running = false to signal send task to exit
    - [ ] Give semaphore to wake send task so it can exit: `xSemaphoreGive(sess->send_sem)`
    - [ ] Wait for task to exit: `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)`
    - [ ] Delete task: `vTaskDelete(sess->send_task_handle)`
    - [ ] Delete semaphore: `vSemaphoreDelete(sess->send_sem)`
    - [ ] Unsubscribe from buffers: `circular_buffer_unsubscribe(temp_sensor_get_air_buffer(), sess);`
    - [ ] Unsubscribe from buffers: `circular_buffer_unsubscribe(temp_sensor_get_heater_buffer(), sess);`
    - [ ] Call `linked_list_destroy(&sess->pending_msgs)` to free all pending messages

---

## Web (React) Changes

### 6. Update Data Collection (App.jsx)

- [ ] **File:** `web/src/App.jsx`
- [ ] **Changes:**
  - [ ] **Remove sliding window:**
    - [ ] Remove `SLIDING_WINDOW_SIZE` constant
    - [ ] Change `setAirData` to accumulate ALL data (no slice)
    - [ ] Change `setHeaterData` to accumulate ALL data (no slice)
  - [ ] **Handle initial historical data:**
    - [ ] When receiving initial data (array with multiple points), process all points
    - [ ] Handle the case where initial data contains both air and heater readings
  - [ ] **Add granularity state:**
    - [ ] Add: `const [granularity, setGranularity] = useState('second');`
    - [ ] Add granularity options: `'second'`, `'minute'`, `'10min'`, `'hour'`
  - [ ] **Add granularity selector UI:**
    - [ ] Add ToggleButtonGroup or Select component
    - [ ] Options: "1s", "1m", "10m", "1h"
  - [ ] **Pass data through aggregation:**
    - [ ] Import `useAggregatedData` hook
    - [ ] Transform raw data before passing to charts

### 7. Create Aggregation Hook

- [ ] **File:** `web/src/hooks/useAggregatedData.js` (new file)
- [ ] **Changes:**
  - [ ] Create hook: `export const useAggregatedData = (data, granularity) => { ... }`
  - [ ] Implement `aggregateBySecond(data)` - return raw data unchanged
  - [ ] Implement `aggregateByMinute(data)`:
    - [ ] Group by minute (timestamp // 60000)
    - [ ] Calculate average temperature per minute bucket
    - [ ] Return: `[{ x: bucket_timestamp, y: avg_temperature }]`
  - [ ] Implement `aggregateBy10Min(data)`:
    - [ ] Group by 10-minute window (timestamp // 600000)
    - [ ] Calculate average per bucket
    - [ ] Return aggregated data
  - [ ] Implement `aggregateByHour(data)`:
    - [ ] Group by hour (timestamp // 3600000)
    - [ ] Calculate average per bucket
    - [ ] Return aggregated data
  - [ ] Return appropriate aggregated data based on `granularity` parameter

---

## Data Format

### WebSocket Message Format

**Initial historical data (on subscribe):**
- Sent as separate messages for air and heater (NOT interleaved)
- Each message contains up to 100 readings (LINKED_LIST_CHUNK_SIZE)

Air buffer message:
```json
[
  {"sensor": "air", "temperature": 31.71, "timestamp": 1700000000000},
  {"sensor": "air", "temperature": 31.72, "timestamp": 1700000001000},
  ...
]
```

Heater buffer message:
```json
[
  {"sensor": "heater", "temperature": 23.05, "timestamp": 1700000000000},
  {"sensor": "heater", "temperature": 23.06, "timestamp": 1700000001000},
  ...
]
```

**New data point (on push):**
```json
[{"sensor": "air", "temperature": 31.82, "timestamp": 1700000002000}]
```

### Chart Data Format

```javascript
// Raw (second granularity)
[{ x: 1700000000000, y: 31.71 }, { x: 1700000001000, y: 31.72 }, ...]

// Aggregated (minute granularity)
[{ x: 1700000000000, y: 31.715 }, { x: 1700000060000, y: 32.1 }, ...]
// x = start of minute bucket, y = average temperature
```

---

## Testing Checklist

### ESP32
- [ ] Build succeeds with no errors
- [ ] Flash to device successfully
- [ ] Monitor shows circular buffer initialization with new size
- [ ] WebSocket connection established
- [ ] Historical data sent on connection (verify count matches buffer size)
- [ ] New temperature readings broadcast to connected clients

### Web
- [ ] `npm run dev` starts successfully
- [ ] Page loads without JavaScript errors
- [ ] WebSocket connects and receives initial data
- [ ] Charts display data correctly
- [ ] Granularity selector visible and functional
- [ ] Switching granularity updates charts correctly
- [ ] Data at 1-second resolution shows all points
- [ ] Data at minute resolution shows averaged points
- [ ] Data at 10-minute resolution shows averaged points
- [ ] Data at hour resolution shows averaged points

---

## Implementation Order

1. Increase `TEMP_BUFFER_SIZE` in `include/temp.h`
2. Add subscriber system and `circular_buffer_iterate()` to `circular_buffer.h` and `circular_buffer.c`
3. Add unit tests for circular buffer subscriber system
4. Run circular buffer subscriber tests: `.\run_unit_tests.bat`
5. Add buffer accessors to `temp.h` and `temp.c`
6. Create linked list module: `include/linked_list.h` and `main/linked_list.c`
7. Add linked list tests: `docker_tests/unit_tests/main/test_linked_list.c`
8. Add linked list test file to `docker_tests/unit_tests/CMakeLists.txt`
9. Run linked list tests: `.\run_unit_tests.bat`
10. Add linked list to ESP32 build: add `main/linked_list.c` to `main/CMakeLists.txt`
11. Update `web_websocket.c` to subscribe to circular buffers, use linked list, and implement send task with semaphore
12. Build and test ESP32
13. Create `useAggregatedData.js` hook
14. Update `App.jsx` for data collection and granularity
15. Build and test web application
16. Verify end-to-end functionality

---

## Notes

- Memory management: WebSocket session allocates PSRAM for its copy of data, frees on disconnect
- No persistence: Data is lost on page refresh (by design, as per requirements)
- Timestamp format: Milliseconds since epoch (Unix timestamp in ms)
- Aggregation: Simple average of all points in each time bucket
- Linked list: Generic implementation with max_size parameter, thread-safe with mutex
- Chunking: Initial payload broken into 100-reading chunks (LINKED_LIST_CHUNK_SIZE)
- Serialization: JSON conversion happens after dequeue, right before sending
- Queue full: When linked list is full, enqueue fails with warning (old messages preserved)
- Initial data: Sent as separate messages for air and heater (not interleaved)
- Send task: Uses binary semaphore to wake when data is enqueued, blocks when queue is empty
- Circular buffer iteration: Uses callback pattern to iterate all elements under mutex lock
