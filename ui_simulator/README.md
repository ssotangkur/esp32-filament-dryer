# UI Simulator for ESP32 Filament Dryer

This directory contains a web-based UI simulator that allows you to develop and test the user interface without needing to flash the ESP32. The simulator compiles the same UI code to WebAssembly using Emscripten.

## Prerequisites

- Docker Desktop (for Docker-based build)
- OR Emscripten SDK (emscripten) for manual build
- CMake
- SDL2 development libraries

## Building the Simulator

### Recommended: Docker-based Build (No Local Emscripten Installation Required)

The Docker approach eliminates the need to install Emscripten locally by running the build in a container with all necessary tools pre-installed.

#### On Linux/Mac:
```bash
# From project root
./ui_simulator/build_docker.sh
```

#### On Windows:
```cmd
# From project root
.\ui_simulator\build_docker.bat
```

### Alternative: Manual Build (Requires Local Emscripten Installation)

If you prefer to build locally without Docker, you'll need to install Emscripten manually:

#### Installing Emscripten

Unlike other dependencies like LVGL, Emscripten cannot be installed as a simple project dependency because it's a complete toolchain that needs to be installed at the system level. You'll need to install it separately on your system:

##### On Windows:
1. Download the Emscripten SDK from: https://github.com/emscripten-core/emsdk
2. Extract it to a directory (e.g., `C:\emsdk`)
3. Open Command Prompt and run:
   ```cmd
   cd C:\emsdk
   git pull
   emsdk install latest
   emsdk activate latest
   emsdk_env.bat
   ```
4. You may want to add the emsdk environment permanently by running:
   ```cmd
   emsdk construct_env
   ```

##### On Linux/Mac:
1. Clone the Emscripten SDK:
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ```
2. Install and activate the latest version:
   ```bash
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```
3. To permanently add to your shell, add this to your `.bashrc` or `.zshrc`:
   ```bash
   source "/path/to/emsdk/emsdk_env.sh"
   ```

#### Building with Manual Installation:

##### On Linux/Mac:
```bash
# From project root
./ui_simulator/build_sim.sh
```

##### On Windows:
```cmd
# From project root
.\ui_simulator\build_sim.bat
```

## Running the Simulator

After building, open `ui_simulator/build/index.html` in a web browser to run the simulator.

## How It Works

The simulator:

1. Uses the same UI code from `main/ui/` directory
2. Provides stub implementations for hardware-specific functions
3. Uses SDL2 to simulate the display in the browser
4. Compiles the code to WebAssembly using Emscripten

## Development Workflow

1. Make changes to the UI code in `main/ui/`
2. Run the Docker build script to update the simulator
3. Refresh the browser to see the changes
4. Once satisfied, test on the actual ESP32 hardware

## Limitations

- Hardware-specific functionality is stubbed out
- Performance may differ from actual hardware
- Some hardware interactions may behave differently

## Files

- `src/main.c` - Main entry point for the simulator
- `src/ui_stub.h` and `src/ui_stub.c` - Stub implementations for hardware functions
- `build_docker.sh` - Docker build script for Linux/Mac
- `build_docker.bat` - Docker build script for Windows
- `build_sim.sh` - Legacy build script for Linux/Mac (requires local Emscripten)
- `build_sim.bat` - Legacy build script for Windows (requires local Emscripten)
- `Dockerfile` - Docker image configuration
- `docker-compose.yml` - Docker Compose configuration
- `CMakeLists.txt` - CMake configuration
- `shell.html` - HTML template for the web page
