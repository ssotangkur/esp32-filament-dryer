# CMakeLists.txt for UI Simulator
# This file configures the build for the UI simulator using Emscripten

cmake_minimum_required(VERSION 3.12)
project (ui_simulator)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -s USE_SDL=2")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/examples)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/demos)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Add LVGL subdirectory to build it
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl lvgl)

# Define source files - include both the simulator files and our UI files
file(GLOB_RECURSE MY_SOURCES "/project/ui_simulator/src/*.c")
file(GLOB_RECURSE UI_SOURCES "/project/main/ui/*.c")
set(SOURCES ${MY_SOURCES} ${UI_SOURCES})

# Define BUILD_FOR_SIMULATOR for all source files
add_compile_definitions(BUILD_FOR_SIMULATOR=1)

add_executable(index ${SOURCES} ${INCLUDES})

# Use our filament dryer UI initialization
set(LVGL_CHOSEN_DEMO init_ui)
set_source_files_properties(main.c PROPERTIES COMPILE_FLAGS -DCHOSEN_DEMO=${LVGL_CHOSEN_DEMO})

# Set executable suffix to html
set(CMAKE_EXECUTABLE_SUFFIX ".html")

target_link_libraries(index
    lvgl
    lvgl_examples
    lvgl_demos
    lvgl_thorvg
    SDL2
)

set_target_properties(index PROPERTIES LINK_FLAGS "--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html -s SINGLE_FILE=1")