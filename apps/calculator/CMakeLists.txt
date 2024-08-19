# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Select board configuration based on -DBUILD_BOARD
if(NOT DEFINED BUILD_BOARD)
    message(FATAL_ERROR "BUILD_BOARD CMake variable is not set")
endif()

set(ENV{BUILD_BOARD} ${BUILD_BOARD})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(calculator)
