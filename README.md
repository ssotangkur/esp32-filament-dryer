# ESP32 Filament Dryer

An ESP32-based filament dryer project using the LilyGo T-Display S3 board.

## Features

- Temperature monitoring and control
- WiFi connectivity for remote monitoring
- Display interface for local control
- System diagnostics and monitoring

## Hardware

- **Board**: LilyGo T-Display S3
- **Microcontroller**: ESP32-S3
- **Display**: 1.9-inch TFT LCD (170x320, ST7789V)
- **Power**: LiPo battery support

## Setup

### Prerequisites

- ESP-IDF development environment
- Visual Studio Code with ESP-IDF extension
- LilyGo T-Display S3 board
- Python 3.x (for version management scripts)

### Building with Automatic Version Increment

This project includes automatic patch version incrementing for each build:

```bash
# Recommended: Use the automated build script (increments version + builds + updates metadata)
scripts\build_with_version_increment.bat

# Manual version increment
python scripts\increment_version.py

# Manual build (traditional way)
esp_idf_shell.bat idf.py build
```

Each time you run the automated build script, the patch version automatically increments (1.0.1 → 1.0.2 → 1.0.3, etc.), ensuring every build has a unique version number for reliable OTA updates.

### WiFi Credentials Setup

This project requires WiFi credentials to connect to your network. Credentials are kept private and not committed to version control.

1. **Copy the credentials template:**
   ```bash
   cp include/wifi_credentials.h.template include/wifi_credentials.h
   ```

2. **Edit the credentials file:**
   Open `include/wifi_credentials.h` and replace the placeholder values:
   ```c
   #define WIFI_SSID "YOUR_ACTUAL_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_ACTUAL_WIFI_PASSWORD"
   #define OTA_URL "http://YOUR_LOCAL_SERVER_IP/firmware.bin"
   ```

3. **Important Notes:**
   - The `include/wifi_credentials.h` file is automatically ignored by Git (see `.gitignore`)
   - Never commit your actual credentials to the repository
   - Each developer should create their own `wifi_credentials.h` file

### Building and Flashing

1. **Configure the project:**
   ```bash
   esp_idf_shell.bat idf.py menuconfig
   ```
   Note: Avoid using menuconfig for WiFi settings - use the credentials file instead.

2. **Build the project:**
   ```bash
   esp_idf_shell.bat idf.py build
   ```

3. **Flash to device:**
   ```bash
   esp_idf_shell.bat idf.py flash
   ```

4. **Monitor output:**
   ```bash
   esp_idf_shell.bat idf.py monitor
   ```

Or combine build, flash, and monitor:
```bash
esp_idf_shell.bat idf.py build flash monitor
```

## Project Structure

- `include/` - Header files
- `main/` - Source files and main application logic
- `scripts/` - Build and version management scripts
- `build/esp32s3/` - Build artifacts and metadata
  - `firmware.bin` - Compiled firmware binary
  - `version.json` - Build metadata and version information
- `managed_components/` - ESP-IDF managed components

## Build Information and Versioning

This project captures comprehensive build metadata for traceability and debugging:

### Build Metadata Locations

**1. Version JSON File** (`build/esp32s3/version.json`):
```json
{
  "version": "1.0.2",
  "build_date": "2025-12-29",
  "git_commit": "46ba299",
  "git_version": "1.0.0",
  "description": "ESP32 filament dryer firmware with improved versioning",
  "target": "esp32s3"
}
```

**2. CMake-Generated Header** (`build/include/version.h`):
```c
#define GIT_COMMIT "46ba299"
#define BUILD_DATE "2025-12-29"
#define FIRMWARE_VERSION_STRING "1.0.2"
```

### Version Information Access

The firmware includes runtime access to version information:

```c
#include "version.h"

// Get current version as string
const char* version = get_firmware_version_string(); // "1.0.2"

// Get detailed build information
firmware_info_t info = get_firmware_info();
// info.git_commit = "46ba299"
// info.build_date = "2025-12-29"
// info.target = "esp32s3"
```

### Automatic Version Increment

Each build automatically increments the patch version:
- **Build 1**: `1.0.1`
- **Build 2**: `1.0.2`
- **Build 3**: `1.0.3`

This ensures every firmware binary has a unique version number for reliable OTA updates.

## Over-the-Air (OTA) Updates

This project supports OTA firmware updates for wireless updates without physical access to the device.

### OTA Features

- **Automatic rollback protection**: Failed updates automatically rollback to the previous working version
- **HTTPS support**: Secure firmware downloads
- **Progress tracking**: Monitor update progress programmatically
- **Non-blocking**: Updates run in background tasks

### Using OTA Updates

The device automatically checks for OTA updates every 5 seconds after startup and installs them when available. The OTA system can also be controlled programmatically:

#### Automatic Updates
The firmware automatically:
- Checks for updates every 5 seconds after WiFi connection
- Downloads and installs new firmware versions when found
- Logs all OTA activities to the console
- Waits 30 seconds after update attempts before checking again

#### Manual OTA Control
```c
#include "ota.h"
#include "wifi_credentials.h"

// Initialize OTA (called automatically in app_main)
ota_init();

// Check if an update is available before downloading
if (ota_check_for_update(OTA_URL)) {
    printf("Update available! Current version: %s\n", FIRMWARE_VERSION);

    // Start the update
    ota_update_from_url(OTA_URL);
} else {
    printf("Firmware is up to date\n");
}

// Or update directly without checking (old behavior)
ota_update_from_url(OTA_URL);

// Or use specific functions for HTTP (local network) or HTTPS (internet)
ota_update_from_http_url("http://192.168.1.100/firmware.bin");
ota_update_from_https_url("https://example.com/firmware.bin");

// Check update status
if (ota_is_updating()) {
    int progress = ota_get_progress();
    printf("OTA progress: %d%%\n", progress);
}
```

### Building OTA Firmware

To create an OTA-compatible firmware binary:

1. **Build the project:**
   ```bash
   esp_idf_shell.bat idf.py build
   ```

2. **The firmware binary will be created at:**
   `build/esp32s3/firmware.bin`

### Hosting OTA Firmware

#### Option 1: Local Development Server (Recommended)

This project includes a simple Node.js HTTP server for local development:

1. **Install dependencies:**
   ```bash
   cd ota-server
   npm install
   cd ..
   ```

2. **Start the OTA server:**
   ```bash
   cd ota-server
   npm run serve
   ```
   Or directly:
   ```bash
   node ota-server.js
   ```

3. **The server will automatically detect and display your local IP address and the complete OTA URL.** Copy the URL shown in the terminal output to your `wifi_credentials.h`:
   ```c
   #define OTA_URL "http://192.168.X.X:3001/firmware.bin"
   ```

4. **Server features:**
   - Serves firmware.bin from your ESP32 build directory
   - Provides version info via `/version` endpoint (reads current version.json)
   - Provides status endpoint at `/status`
   - Includes CORS headers for React/Vite integration
   - Web interface at root URL with setup instructions

#### Option 2: External Hosting

Host the `firmware.bin` file on any HTTP/HTTPS server. For OTA update checking to work, the server must provide a `/version` endpoint that returns version information in JSON format.

**Required Server Endpoints:**

1. **Firmware Download**: `GET /firmware.bin`
   - Returns the firmware binary file
   - Must include proper HTTP headers for binary content

2. **Version Information**: `GET /version`
   - Returns JSON with version metadata
   - Required format:
   ```json
   {
     "version": "1.0.2",
     "build_date": "2025-12-29",
     "git_commit": "46ba299",
     "description": "Firmware description"
   }
   ```

**Example Server Implementation (Node.js/Express):**
```javascript
const express = require('express');
const app = express();

// Serve firmware binary
app.get('/firmware.bin', (req, res) => {
  res.sendFile('/path/to/firmware.bin');
});

// Serve version information
app.get('/version', (req, res) => {
  res.json({
    "version": "1.0.2",
    "build_date": "2025-12-29",
    "git_commit": "46ba299",
    "description": "ESP32 filament dryer firmware"
  });
});

app.listen(3000);
```

**OTA URL Configuration:**
Update your `wifi_credentials.h` with the base URL:
```c
#define OTA_URL "https://your-server.com/firmware.bin"
```

The firmware will automatically check `https://your-server.com/version` to determine if an update is available before downloading.

### OTA Partition Layout

The partition table includes:
- `factory`: Initial firmware partition
- `ota_0` & `ota_1`: Alternating update partitions
- `ota_data`: Stores which partition is active

## Development

- Use `esp_idf_shell.bat` prefix for all ESP-IDF commands
- Credentials are separated into `include/wifi_credentials.h` (gitignored)
- Template provided at `include/wifi_credentials.h.template`
- OTA functionality is automatically initialized after WiFi connection

## License

See LICENSE file for details.
