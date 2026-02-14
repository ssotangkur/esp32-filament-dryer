# Web Development & Debugging Workflow

This file documents the workflow for debugging web-related issues in this ESP32 Filament Dryer project.

## Device Configuration

- **Device IP Address:** `192.168.2.18`
- **HTTP Server Port:** `3000`
- **WebSocket Endpoint:** `ws://192.168.2.18:3000/ws/sensor-data`

## Accessing the Web UI

- **Development Server:** `http://localhost:5173` (Vite dev server)
- **Production (via ESP32):** `http://192.168.2.18:3000`

## WebSocket Data Format

### Initial Data Request (`get_data`)
```json
[{"sensor":"air","temperature":31.71,"timestamp":332660},{"sensor":"heater","temperature":23.05,"timestamp":332660}]
```

### Continuous Updates
Individual sensor readings are broadcast:
```json
{"sensor":"air","temperature":31.82,"timestamp":16580}
{"sensor":"heater","temperature":23.68,"timestamp":16620}
```

## Chrome DevTools MCP Tools

Use these tools for browser debugging:

| Tool | Purpose |
|------|---------|
| `chrome-devtools_list_pages` | List open browser pages |
| `chrome-devtools_select_page` | Select a page for interaction |
| `chrome-devtools_navigate_page` | Navigate to URL or reload |
| `chrome-devtools_take_snapshot` | Get accessibility tree of page |
| `chrome-devtools_take_screenshot` | Take screenshot of page |
| `chrome-devtools_list_console_messages` | View console logs and errors |
| `chrome-devtools_list_network_requests` | View network requests |

### Common Commands

```javascript
// Reload the page
chrome-devtools_navigate_page(type="reload")

// Get page snapshot
chrome-devtools_take_snapshot()

// View console messages
chrome-devtools_list_console_messages()

// Navigate to URL
chrome-devtools_navigate_page(type="url", url="http://localhost:5173")
```

## Known Issues

### Vite WebSocket Proxy Not Working
The Vite dev server WebSocket proxy (`/ws` -> `ws://192.168.2.18:3000`) does not work reliably. 

**Workaround:** The React app connects directly to the ESP32 IP address (`ws://192.168.2.18:3000`) instead of going through the proxy.

### Initial Sensor Data Missing Timestamps
The initial `send_sensor_data_json` response previously did not include timestamps, causing chart rendering errors (NaN). This has been fixed to include timestamps.

## Example Debugging Workflow

This section documents how a recent issue was investigated and fixed.

### Problem Statement
The webpage was only receiving the first datapoint from the WebSocket, not continuous updates every second.

### Investigation Steps

1. **Analyze the codebase**
   - Used `grep` to find WebSocket handler code (`main/web/web_websocket.c`)
   - Used `grep` to find temperature subject setters (`main/ui/subjects.c`)
   - Used `grep` to find how temperature sensors publish data (`main/temp.c`)

2. **Check browser state with Chrome DevTools MCP**
   ```bash
   # View page console for WebSocket errors
   chrome-devtools_list_console_messages()
   
   # Get page snapshot to see current state
   chrome-devtools_take_snapshot()
   ```
   - Found WebSocket was connecting but closing immediately
   - Found chart errors (NaN) due to missing timestamps

3. **Add ESP32 debug logging**
   - Added `ESP_LOGI` to `ws_broadcast_data()` to track broadcasts
   - Added `ESP_LOGI` to `subjects_set_air_temp()` to track temperature updates
   - Used `@idf` subagent to build and flash

4. **Monitor ESP32 output**
   ```bash
   @idf monitor
   ```
   - Found temperature updates happening: `Setting air temp: 31.84`
   - Found broadcasts happening: `ws_broadcast_data: broadcasting air=31.84`
   - But broadcast count was 0 clients!

5. **Identify root cause**
   - The LVGL observer callbacks (`air_temp_subject_callback`, `heater_temp_subject_callback`) were registered but never being triggered
   - This appears to be an issue with calling `lv_subject_set_float()` from a non-LVGL FreeRTOS task

6. **Implement fix**
   - Modified `subjects_set_air_temp()` and `subjects_set_heater_temp()` in `main/ui/subjects.c` to directly call `ws_broadcast_data()` after setting the subject
   - Added timestamps to initial sensor data response in `send_sensor_data_json()`
   - Made `ws_broadcast_data()` non-static to allow calling from subjects.c
   - Updated React app to connect directly to ESP32 IP with reconnection logic

7. **Verify fix**
   - Built and flashed with `@idf build flash`
   - Monitored with `@idf monitor` - saw broadcasts going to clients
   - Checked browser console - saw WebSocket messages arriving every second
   - Verified charts displayed real-time temperature data

### Tools Used Summary

| Phase | Tools/Agents Used |
|-------|-------------------|
| Analysis | `grep`, `glob`, `read` (file exploration) |
| Build/Flash | `@idf build flash` |
| Monitor ESP | `@idf monitor`, read log files |
| Browser Debug | `chrome-devtools_list_console_messages`, `chrome-devtools_take_snapshot` |
| Code Changes | `edit`, `write` |
| Commit | `@git_commit_assistant` |

### Key Insights

1. **LVGL observers don't fire from non-LVGL tasks**: When `lv_subject_set_float()` is called from a FreeRTOS task other than the LVGL task, the observer callbacks may not trigger. This is likely a threading/notification issue within LVGL.

2. **Direct broadcast is the workaround**: Call the broadcast function directly from the temperature setter rather than relying on LVGL observers.

3. **Vite proxy issues**: The Vite dev server WebSocket proxy can be unreliable; connecting directly to the device IP is more reliable.

4. **Timestamps are critical**: Always include timestamps in WebSocket data for time-series charts.
