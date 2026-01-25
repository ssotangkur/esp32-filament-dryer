---
name: esp-idf-workflow
description: Detailed ESP-IDF command usage and environment setup procedures
---

## ESP-IDF Workspace Rules

### ESP-IDF Command Usage

ESP-IDF commands like `idf.py`, `esptool.py`, etc. must be run through the `esp_idf_shell.bat` script to properly set up the environment.

#### Correct Usage:
```bash
esp_idf_shell.bat idf.py build
esp_idf_shell.bat idf.py flash
esp_idf_shell.bat idf.py monitor
```

#### Why this is necessary:
1. The `esp_idf_shell.bat` script sets up required environment variables
2. It configures the correct Python environment
3. It establishes proper PATH settings for ESP-IDF tools
4. It creates DOSKEY macros for ESP-IDF commands

#### Common Commands:
- Build: `esp_idf_shell.bat idf.py build`
- Flash: `esp_idf_shell.bat idf.py flash`
- Monitor: `esp_idf_shell.bat idf.py monitor`
- Full build & flash: `esp_idf_shell.bat idf.py build flash monitor`

#### Error Prevention:
Running ESP-IDF commands directly (e.g., `idf.py build`) will result in:
```
'idf.py' is not recognized as an internal or external command
```

Always use `esp_idf_shell.bat` as a prefix to ESP-IDF commands.

## Interactive Commands to Avoid

### Menuconfig
Do not run `idf.py menuconfig` or `esp_idf_shell.bat idf.py menuconfig` as these commands are interactive and require user input through a terminal-based menu interface.

### Why avoid menuconfig:
1. The menuconfig interface requires interactive navigation with arrow keys, enter, and other inputs
2. As an AI assistant, I cannot interact with or navigate the menuconfig interface
3. Running menuconfig will cause the command to hang waiting for user input
4. Configuration changes should be made by directly editing `sdkconfig` or `sdkconfig.defaults` files

### Alternative approaches:
- Edit `sdkconfig` or `sdkconfig.defaults` directly with desired CONFIG_xxx=y settings
- Use `idf.py save-defconfig` to save current configuration to defaults
- Use `idf.py reconfigure` to apply configuration changes