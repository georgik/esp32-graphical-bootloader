# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Select board configuration based on -DBUILD_BOARD
if(NOT DEFINED BUILD_BOARD)
    message(FATAL_ERROR "BUILD_BOARD CMake variable is not set")
endif()

set(ENV{USE_ESP_BOX} "0")
set(ENV{USE_ESP_BOX_3} "0")
set(ENV{USE_M5STACK_CORE_S3} "0")
set(ENV{USE_ESP32_P4_FUNCTION_EV_BOARD} "0")

if (BUILD_BOARD STREQUAL "esp-box")
    set(ENV{USE_ESP_BOX} "esp32s3")
elseif (BUILD_BOARD STREQUAL "esp-box-3")
    set(ENV{USE_ESP_BOX_3} "esp32s3")
elseif (BUILD_BOARD STREQUAL "m5stack_core_s3")
    set(ENV{USE_M5STACK_CORE_S3} "esp32s3")
elseif (BUILD_BOARD STREQUAL "esp32_p4_function_ev_board")
    set(ENV{USE_ESP32_P4_FUNCTION_EV_BOARD} "esp32p4")
endif()

set(COMPONENTS main) # "Trim" the build. Include the minimal set of components; main and anything it depends on.

# Each configuration's sdkconfig will be in the build directory
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-graphical-bootloader)
