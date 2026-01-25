# CMakeLists.txt for UI Simulator
# This file configures the build for the UI simulator using Emscripten

cmake_minimum_required(VERSION 3.16)

# Project name
project(ui_simulator)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/examples)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/demos)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../main/ui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
# Add the project root directory to include the generated lv_conf.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Define build flag for simulator
add_definitions(-DBUILD_FOR_SIMULATOR)

# Define source files
set(SOURCES
    src/main.c
    src/ui_stub.c
)

# Add executable
add_executable(ui_simulator ${SOURCES})

# Link libraries - using lvgl directly from the managed components
target_link_libraries(ui_simulator 
    SDL2 
    SDL2main
)

# Emscripten-specific settings
if(EMSCRIPTEN)
    # Set Emscripten-specific linker flags
    set_target_properties(ui_simulator PROPERTIES 
        LINK_FLAGS "--bind -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY -s MODULARIZE -s EXPORT_NAME='Module' -s ENVIRONMENT=web,shell -s NO_EXIT_RUNTIME=1 -s SUPPORT_LONGJMP=0 -s WASM=1 --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
    )
    
    # Set output name for web target
    set_target_properties(ui_simulator PROPERTIES OUTPUT_NAME "index")
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(ui_simulator PRIVATE /W4)
else()
    target_compile_options(ui_simulator PRIVATE -Wall -Wextra -pedantic)
endif()