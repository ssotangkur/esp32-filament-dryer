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

- [x] **File:** `include/circular_buffer.h` - **COMMITTED**
- [x] **Changes:**
  - [x] Add subscriber callback type
  - [x] Add subscriber struct
  - [x] Add to `circular_buffer_t`: subscribers, subscriber_count
  - [x] Add function: `circular_buffer_subscribe()`
  - [x] Add function: `circular_buffer_unsubscribe()`
  - [x] Add function: `circular_buffer_iterate()`

- [x] **File:** `main/circular_buffer.c` - **COMMITTED**
- [x] **Changes:**
  - [x] Implement `circular_buffer_subscribe()` - allocates subscribers in PSRAM, sends historical data
  - [x] Implement `circular_buffer_unsubscribe()` - removes subscriber, reallocates array
  - [x] Implement `circular_buffer_iterate()` - iterates all elements with callback
  - [x] Modify `circular_buffer_push()` to notify subscribers
  - [x] **Add unit tests for subscriber system:**
    - [x] **File:** `docker_tests/unit_tests/main/test_circular_buffer.c` - **10 new tests**
    - [x] test_circular_buffer_subscribe_empty_buffer
    - [x] test_circular_buffer_subscribe_with_data
    - [x] test_circular_buffer_subscribe_null_params
    - [x] test_circular_buffer_unsubscribe
    - [x] test_circular_buffer_unsubscribe_not_found
    - [x] test_circular_buffer_multiple_subscribers
    - [x] test_circular_buffer_push_notifies_subscribers
    - [x] test_circular_buffer_iterate
    - [x] test_circular_buffer_iterate_empty
    - [x] test_circular_buffer_iterate_null_params
    - [x] All tests passing (verified with `.\run_unit_tests.bat`)

### 3. Add Temperature Buffer Accessors

- [x] **File:** `include/temp.h` - **COMMITTED**
- [x] **Changes:**
  - [x] Add: `circular_buffer_t* temp_sensor_get_air_buffer(void);`
  - [x] Add: `circular_buffer_t* temp_sensor_get_heater_buffer(void);`

- [x] **File:** `main/temp.c` - **COMMITTED**
- [x] **Changes:**
  - [x] Implement `temp_sensor_get_air_buffer()` - return `&temp_buffer_1`
  - [x] Implement `temp_sensor_get_heater_buffer()` - return `&temp_buffer_2`

### 4. Create Linked List Module for Pending Messages

- [x] **File:** `include/linked_list.h` (new file) - **COMMITTED**
- [x] **Changes:**
  - [x] Add node structure
  - [x] Add list structure
  - [x] Add chunk size constant: `LINKED_LIST_CHUNK_SIZE 100`
  - [x] Add function declarations: create, enqueue, peek_head, peek_tail, pop_head, pop_tail, destroy, count

- [x] **File:** `main/linked_list.c` (new file) - **COMMITTED**
- [x] **Changes:**
  - [x] Add includes
  - [x] Implement `linked_list_create()`
  - [x] Implement `linked_list_enqueue()` - stores pointer directly (no copy)
  - [x] Implement `linked_list_peek_head()`
  - [x] Implement `linked_list_peek_tail()`
  - [x] Implement `linked_list_pop_head()`
  - [x] Implement `linked_list_pop_tail()`
  - [x] Implement `linked_list_destroy()`
  - [x] Implement `linked_list_count()`
  - [x] **Add unit tests for linked list:**
    - [x] **File:** `docker_tests/unit_tests/main/test_linked_list.c` (new file) - **25 comprehensive tests**
    - [x] All tests passing (verified with `.\run_unit_tests.bat`)
    - [x] Mock wrapper pattern documented in `DOCKER_TESTING_README.md`

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

- [x] **File:** `web/src/App.jsx` - **COMMITTED**
- [x] **Changes:**
  - [x] **Remove sliding window:**
    - [x] Removed `SLIDING_WINDOW_SIZE` constant
    - [x] `setAirData` accumulates ALL data (no slice)
    - [x] `setHeaterData` accumulates ALL data (no slice)
  - [x] **Handle initial historical data:**
    - [x] Receives array with multiple points, processes all points
  - [x] **Add granularity state:**
    - [x] `const [granularity, setGranularity] = useState('second');`
    - [x] Granularity options: `'second'`, `'minute'`, `'10min'`, `'hour'`
  - [x] **Add granularity selector UI:**
    - [x] ToggleButtonGroup with options: "1s", "1m", "10m", "1h"
  - [x] **Pass data through aggregation:**
    - [x] Import `useAggregatedData` hook
    - [x] Transform raw data before passing to charts

### 7. Create Aggregation Hook

- [x] **File:** `web/src/hooks/useAggregatedData.js` (new file) - **COMMITTED**
- [x] **Changes:**
  - [x] Create hook: `export const useAggregatedData = (data, granularity)`
  - [x] Implement `aggregateBySecond(data)` - return raw data unchanged
  - [x] Implement `aggregateByMinute(data)` - group by minute, calculate average
  - [x] Implement `aggregateBy10Min(data)` - group by 10-minute window, calculate average
  - [x] Implement `aggregateByHour(data)` - group by hour, calculate average
  - [x] Return appropriate aggregated data based on `granularity` parameter
  - [x] Uses `useMemo` for performance optimization

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

1. [x] Increase `TEMP_BUFFER_SIZE` in `include/temp.h` - **COMMITTED (7200 = 2 hours)**
2. [x] Add subscriber system and `circular_buffer_iterate()` to `circular_buffer.h` and `circular_buffer.c` - **COMMITTED**
   - [x] Header changes committed
   - [x] Implementation in `main/circular_buffer.c` - **COMMITTED**
3. [x] Add unit tests for circular buffer subscriber system - **COMMITTED (10 tests)**
4. [x] Run circular buffer subscriber tests: `.\run_unit_tests.bat` - **ALL 60 TESTS PASSING**
5. [x] Add buffer accessors to `temp.h` and `temp.c` - **COMMITTED**
6. [x] Create linked list module: `include/linked_list.h` and `main/linked_list.c` - **COMMITTED**
7. [x] Add linked list tests: `docker_tests/unit_tests/main/test_linked_list.c` - **COMMITTED (25 tests)**
8. [x] Add linked list test file to `docker_tests/unit_tests/CMakeLists.txt` - **COMMITTED**
9. [x] Run linked list tests: `.\run_unit_tests.bat` - **ALL TESTS PASSING**
10. [x] Add linked list to ESP32 build: auto-included by GLOB_RECURSE - **COMMITTED**
11. [x] Update `web_websocket.c` to subscribe to circular buffers, use linked list, and implement send task with semaphore - **COMMITTED**
12. [x] Build and test ESP32 - **COMMITTED (build successful)**
13. [x] Create `useAggregatedData.js` hook - **COMMITTED**
14. [x] Update `App.jsx` for data collection and granularity - **COMMITTED**
15. [x] Build and test web application - **COMMITTED (built into LittleFS image)**
16. [ ] Verify end-to-end functionality - **TODO (requires device flash and testing)**

---

## Current Progress Summary

**Completed (committed):**
- ✅ TEMP_BUFFER_SIZE set to 7200 (2 hours) in `include/temp.h`
- ✅ Circular buffer subscriber system (`include/circular_buffer.h`, `main/circular_buffer.c`)
- ✅ Circular buffer subscriber unit tests (10 tests in `test_circular_buffer.c`)
- ✅ Temperature buffer accessors (`temp_sensor_get_air_buffer()`, `temp_sensor_get_heater_buffer()`)
- ✅ Linked list module (`include/linked_list.h`, `main/linked_list.c`)
- ✅ Linked list unit tests (25 tests in `test_linked_list.c`)
- ✅ Web aggregation hook (`web/src/hooks/useAggregatedData.js`)
- ✅ Web App.jsx with granularity selector and data accumulation
- ✅ Mock wrapper pattern documentation (`DOCKER_TESTING_README.md`)
- ✅ WebSocket handler updated (`main/web/web_websocket.c`)
  - Subscribes to air and heater temperature buffers on connection
  - Sends historical data in chunks (100 readings per message)
  - Uses linked list for pending messages with PSRAM allocation
  - Send task with semaphore for efficient sleep/wake
  - Proper cleanup on disconnect
- ✅ Firmware build successful with web app embedded in LittleFS
- ✅ All 60 unit tests passing

**Remaining work:**
- ❌ Flash firmware to device and test end-to-end functionality
- ❌ Verify WebSocket data streaming with historical + real-time data
- ❌ Test granularity switching in browser

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
