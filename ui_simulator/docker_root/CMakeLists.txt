# CMakeLists.txt for UI Simulator
# This file configures the build for the UI simulator using Emscripten

cmake_minimum_required(VERSION 3.12)
project (ui_simulator)

# Use -O0 for faster builds during development, -O2 for release
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -s USE_SDL=2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -s USE_SDL=2")

message(STATUS "Using pre-built LVGL libraries from /usr/local/lib")

# Include simulator stubs first (before real ESP-IDF headers)
# This allows ui.c to use ESP-IDF APIs without conditional compilation
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# lv_conf.h needs to be next to the lvgl folder for proper includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../managed_components)

# Use pre-built libraries (including demos and examples)
# These are built during Docker image creation for fast incremental builds
add_library(lvgl STATIC IMPORTED)
set_target_properties(lvgl PROPERTIES IMPORTED_LOCATION "/usr/local/lib/liblvgl.a")

add_library(lvgl_thorvg STATIC IMPORTED)
set_target_properties(lvgl_thorvg PROPERTIES IMPORTED_LOCATION "/usr/local/lib/liblvgl_thorvg.a")

# Include demos and examples libraries
add_library(lvgl_demos STATIC IMPORTED)
set_target_properties(lvgl_demos PROPERTIES IMPORTED_LOCATION "/usr/local/lib/liblvgl_demos.a")

add_library(lvgl_examples STATIC IMPORTED)
set_target_properties(lvgl_examples PROPERTIES IMPORTED_LOCATION "/usr/local/lib/liblvgl_examples.a")

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
    lvgl_thorvg
    lvgl_demos
    lvgl_examples
    SDL2
)

set_target_properties(index PROPERTIES LINK_FLAGS "--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html -s SINGLE_FILE=1")