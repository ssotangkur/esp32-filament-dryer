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
- `example/` - Example configurations
- `managed_components/` - ESP-IDF managed components

## Development

- Use `esp_idf_shell.bat` prefix for all ESP-IDF commands
- Credentials are separated into `include/wifi_credentials.h` (gitignored)
- Template provided at `include/wifi_credentials.h.template`

## License

See LICENSE file for details.
