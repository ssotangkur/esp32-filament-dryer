# UI Simulator Build Fix Plan

## Problem
The UI simulator build is failing because:
1. `LV_USE_DEMO_WIDGETS` is disabled (set to 0) in lv_conf.h
2. main.c is trying to call `lv_demo_widgets()` which doesn't exist since widgets demo is disabled
3. CMakeLists.txt sets `LVGL_CHOSEN_DEMO init_ui` but main.c ignores this and hardcodes the call
4. Additional issues: main.c is missing proper include for our filament dryer UI files
5. UI source files from main/ui directory are not being included in the build

## Solution
1. Update main.c to use conditional compilation based on the CHOSEN_DEMO define to call the appropriate demo/UI function
2. Update CMakeLists.txt to include the UI source files from main/ui directory
3. Add the necessary include for our filament dryer UI header file in main.c

## Specific Changes Needed

### 1. Update main.c line 72
Change from:
```c
lv_demo_widgets();
```

To:
```c
#ifdef CHOSEN_DEMO
  #if CHOSEN_DEMO == init_ui
    init_ui();
  #elif CHOSEN_DEMO == widgets
    lv_demo_widgets();
  #elif CHOSEN_DEMO == benchmark
    lv_demo_benchmark();
  #elif CHOSEN_DEMO == music
    lv_demo_music();
  #else
    lv_demo_widgets();  /* Default fallback */
  #endif
#else
  lv_demo_widgets();  /* Default fallback if not defined */
#endif
```

### 2. Update main.c includes
Add this include after the existing includes:
```c
#include "../main/ui/ui.h"
```

### 3. Update CMakeLists.txt
Modify the SOURCES section to include the UI files:
From:
```cmake
file(GLOB MY_SOURCES "./src/*.c")
set(SOURCES ${MY_SOURCES})
```

To:
```cmake
set(SOURCES
    src/main.c
    src/ui_stub.c
    ../main/ui/ui.c
    ../main/ui/analog_dial.c
)
```

## Expected Result
After these changes, the build should succeed and call our init_ui() function to display the filament dryer UI as intended. The UI files will be properly included in the build and the correct demo/UI function will be called based on the CHOSEN_DEMO setting.

## Implementation Steps for Developer

1. Edit `ui_simulator/src/main.c`:
   - Add `#include "../main/ui/ui.h"` after the existing includes
   - Replace the line `lv_demo_widgets();` (around line 72) with the conditional compilation code shown above

2. Edit `ui_simulator/CMakeLists.txt`:
   - Replace the file(GLOB...) approach with explicit source files as shown above
   - Make sure the SOURCES variable includes all necessary UI files

3. After making these changes, rebuild the simulator using:
   ```
   docker-compose -f ui_simulator/docker-compose.yml up --build
   ```

## CMakeLists.txt Configuration Details

The CMakeLists.txt file needs to be updated to properly include the filament dryer UI source files. Currently it uses a GLOB pattern that may not include the necessary files from the main/ui directory.

### Current CMakeLists.txt sections that need attention:

1. Source files definition (lines 19-20):
   - Current: Uses file(GLOB) to get sources only from ./src/
   - Issue: Doesn't include the UI files from ../main/ui/

2. Target linking (lines 34-40):
   - Currently links lvgl_examples and lvgl_demos libraries
   - May need to ensure proper dependencies for our filament dryer UI

### Recommended CMakeLists.txt changes:

Instead of:
```cmake
file(GLOB MY_SOURCES "./src/*.c")
set(SOURCES ${MY_SOURCES})
```

Use:
```cmake
set(SOURCES
    src/main.c
    src/ui_stub.c
    ../main/ui/ui.c
    ../main/ui/analog_dial.c
)
```

This ensures all required source files are explicitly included in the build.

## Additional Notes
- The current lv_conf.h has LV_USE_DEMO_WIDGETS set to 0, which is why the function doesn't exist
- The CHOSEN_DEMO mechanism is already set up in CMakeLists.txt with `set(LVGL_CHOSEN_DEMO init_ui)`
- The build system is already configured to use our filament dryer UI as the target
- The UI files (ui.c and analog_dial.c) are already available in the main/ui directory

## Verification Steps

After implementing the fixes:

1. Rebuild the simulator:
   ```
   docker-compose -f ui_simulator/docker-compose.yml up --build
   ```

2. Check that the build completes without errors, particularly:
   - No "call to undeclared function 'lv_demo_widgets'" errors
   - All source files compile successfully
   - Final HTML file is generated in ui_simulator/build/

3. Verify the output works correctly:
   - Open ui_simulator/build/index.html in a web browser
   - The filament dryer UI should be displayed
   - Canvas should be sized according to our 320x170 display settings
   - All UI elements should render properly

4. Test functionality:
   - Check that UI controls respond to mouse input
   - Verify any animations or dynamic elements work
   - Confirm the UI behaves as expected for the filament dryer application

## Troubleshooting

If issues persist after applying the fixes:

1. Check that the UI files are actually being compiled by looking at the build output
2. Verify that the init_ui function is properly declared and defined
3. Confirm that all necessary include paths are set up in CMakeLists.txt
4. Make sure there are no conflicting definitions or missing dependencies