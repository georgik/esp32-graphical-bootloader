# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Select board configuration based on -DBUILD_BOARD
if(NOT DEFINED BUILD_BOARD)
    message(FATAL_ERROR "BUILD_BOARD CMake variable is not set. Use idf.py @boards/BOARD.cfg build")
endif()

set(ENV{BUILD_BOARD} ${BUILD_BOARD})

set(COMPONENTS main) # "Trim" the build. Include the minimal set of components; main and anything it depends on.

# Each configuration's sdkconfig will be in the build directory
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-graphical-bootloader)
