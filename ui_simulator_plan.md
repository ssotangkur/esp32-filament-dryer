# UI Simulator Implementation Plan (Docker-enhanced)

## Objective
Create a UI simulator in a `ui_simulator` directory that allows rapid UI development without affecting the main ESP32 project. The simulator will compile the same UI code (`main/ui/ui.c` and `main/ui/analog_dial.c`) to run in a web browser via WebAssembly. The build process will be containerized using Docker to eliminate the need for local Emscripten installation, similar to the existing `docker_tests/` structure.

## Approach
Use Emscripten to compile the UI code to WebAssembly with the `SINGLE_FILE=1` setting to embed WASM directly into HTML, eliminating separate file loading issues. The `SINGLE_FILE=1` flag base64-encodes the WASM binary and embeds it directly into the JavaScript, creating a truly single-file output. Use conditional compilation to handle hardware-specific dependencies. Containerize the build process in a Docker container to make it portable across platforms without requiring local Emscripten installation.

## Dependencies That Need Stubs/Wrappers
From analyzing `main/ui/ui.c`, the hardware-specific dependencies are:
1. `lvgl_port_lock(0)` and `lvgl_port_unlock()` from `esp_lvgl_port.h`
2. `ESP_LOGI()` from `esp_log.h`

## Directory Structure
```
ui_simulator/
├── src/
│   ├── main.c              # Main entry point using SDL
│   ├── ui_stub.h           # Stub declarations for hardware functions
│   └── ui_stub.c           # Stub implementations for hardware functions
├── build/                  # Output directory for generated single HTML file
├── CMakeLists.txt          # CMake configuration for Emscripten
├── Dockerfile              # Docker image with Emscripten and build tools
├── docker-compose.yml      # Docker Compose configuration for build process
├── build_docker.sh         # Docker build script for Linux/Mac
├── build_docker.bat        # Docker build script for Windows
├── build_sim.sh            # Legacy build script (still available)
├── build_sim.bat           # Legacy build script for Windows (still available)
└── README.md               # Updated documentation reflecting Docker approach
```

## Implementation Steps

### Step 1: Create the ui_simulator directory and structure
- Create `ui_simulator/src/` directory
- Create `ui_simulator/build/` directory (for output persistence)

### Step 2: Create stub implementations
- Create `ui_simulator/src/ui_stub.h` - Header with stub declarations
- Create `ui_simulator/src/ui_stub.c` - Stub implementations for hardware functions

### Step 3: Create simulation main file
- Create `ui_simulator/src/main.c` - Main entry point that initializes SDL and LVGL

### Step 4: Create CMakeLists.txt
- Configure for Emscripten compilation with `-s SINGLE_FILE=1`
- Include the necessary LVGL libraries
- Link SDL2 libraries
- **Remove `--shell-file` parameter** to let Emscripten generate default HTML

### Step 5: Create Docker infrastructure
- Create `Dockerfile` - Minimal Ubuntu-based image with Emscripten and build tools
- Create `docker-compose.yml` - Configuration with volume mounting for output persistence
- Follow naming and organizational conventions from `docker_tests/` directory

### Step 6: Create Docker build scripts
- Create `build_docker.sh` - Bash script to run Docker build process on Linux/Mac
- Create `build_docker.bat` - Windows batch file to run Docker build process on Windows
- Follow naming conventions from `docker_tests/` directory

### Step 7: Update documentation
- Update `README.md` to reflect SINGLE_FILE approach as primary method
- Document the benefits of single HTML file output (no separate WASM/JS files)
- Explain how Docker eliminates the need for local Emscripten installation
- Include instructions for both Docker and manual approaches

### Step 8: Test the SINGLE_FILE setup
- Build the simulator using Docker
- Verify the single `index.html` file loads and runs the UI simulator correctly
- Test on both Windows and Linux/Mac systems
- Confirm no WASM loading issues since everything is embedded

## Conditional Compilation Strategy
Use preprocessor directives to conditionally compile hardware-specific code:
- When `BUILD_FOR_SIMULATOR` is defined, use stub implementations
- When compiling for ESP32, use the original hardware functions

## Output Persistence Strategy
- Mount `ui_simulator/build/` directory as a volume from host to container
- Generated **single HTML file** will persist on host filesystem after container stops
- **Single file** will be accessible at `ui_simulator/build/index.html` on the host system
- No separate `.js` or `.wasm` files to manage or load

## SINGLE_FILE Benefits
The `SINGLE_FILE=1` approach provides several advantages:
- **Eliminates WASM loading issues** - WASM is embedded as base64 in the JavaScript
- **Simplifies deployment** - Only need to serve/copy one HTML file
- **Reduces complexity** - No separate file dependencies to manage
- **Better browser compatibility** - Avoids CORS and path resolution issues
- **Follows Emscripten tutorial pattern** - Uses standard, well-documented approach

## Expected Outcome
A **self-contained, single-file UI simulator** that:
- Does not modify the existing UI code
- Runs the same UI code in a browser via WebAssembly
- Provides fast iteration cycle for UI development using Docker containerization
- **Eliminates WASM loading issues** through single HTML file approach
- Works consistently across Windows, Linux, and Mac systems
- Maintains backward compatibility with manual build process
- Does not affect the main ESP32 project
- **Simplifies deployment** with only one HTML file to manage
