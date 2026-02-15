# Web Development & Debugging Workflow

This file provides context for carrying out user requests involving the web interface. Use this information to implement, build, flash, and verify changes end-to-end.

## Implementation Toolkit

These tools support the full workflow from implementation through verification:

| Tool/Action | Use When |
|-------------|----------|
| `@idf build` | Compile ESP32 firmware |
| `@idf flash` | Upload firmware to device |
| `@idf monitor` | View ESP32 serial output |
| `@url_snapshot navigate to <url> and <prompt>` | Load web UI and analyze visual output (screenshot/snapshot based on prompt) |
| `chrome-devtools_list_pages` | List open browser pages |
| `chrome-devtools_select_page` | Select a page for interaction |
| `chrome-devtools_navigate_page` | Navigate to URL or reload |
| `chrome-devtools_take_screenshot` | Visually verify UI renders correctly |
| `chrome-devtools_take_snapshot` | Inspect accessibility tree for data/values |
| `chrome-devtools_list_console_messages` | Check for JavaScript errors |
| `chrome-devtools_list_network_requests` | Verify HTTP/WebSocket connections |

### Example A: UI Styling Changes

**Context:** Changed button styling.

1. Implement code changes
2. Build and flash: `@idf build flash`
3. Wait for device to reboot (~5 seconds)
4. Navigate to web UI and verify styling: `@url_snapshot navigate to http://192.168.2.18:3000 and take a screenshot, then describe the button colors, text, and whether they have rounded corners`
5. Check console for any styling-related errors

### Example B: WebSocket Data Changes

**Context:** Modified WebSocket payload format.

1. Implement code changes
2. Build and flash: `@idf build flash`
3. Wait for device to reboot (~5 seconds)
4. Navigate to web UI and inspect data: `@url_snapshot navigate to http://192.168.2.18:3000 and take a snapshot, then report the air and heater temperature values shown`
5. Check console for WebSocket errors
6. Use `@idf monitor` to verify ESP32 is broadcasting

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
