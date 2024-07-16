cmake_minimum_required(VERSION 3.5)

# List of sub-applications
set(SUB_APP_NAMES
    tic_tac_toe
    wifi_list
    calculator
    synth_piano
    game_of_life
)

# List of corresponding flash addresses
set(SUB_APP_ADDRS
    0x220000
    0x4E0000
    0x7A0000
    0xA60000
    0xD20000
)

# Get the length of each list
list(LENGTH SUB_APP_NAMES LENGTH_SUB_APP_NAMES)
list(LENGTH SUB_APP_ADDRS LENGTH_SUB_APP_ADDRS)

# Ensure both lists have the same length
if(NOT LENGTH_SUB_APP_NAMES EQUAL LENGTH_SUB_APP_ADDRS)
    message(FATAL_ERROR "The number of applications does not match the number of addresses.")
endif()

# Function to build and flash each sub-application
function(build_and_flash_app APP ADDR)
    message(STATUS "Building ${APP}")
    execute_process(
        COMMAND idf.py build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/apps/${APP}
        RESULT_VARIABLE build_result
    )
    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR "Failed to build ${APP}")
    endif()

    message(STATUS "Flashing ${APP} to address ${ADDR}")
    execute_process(
        COMMAND esptool.py --chip esp32s3 --baud 921600 --before default_reset --after hard_reset write_flash ${ADDR} ${CMAKE_SOURCE_DIR}/apps/${APP}/build/${APP}.bin
        RESULT_VARIABLE flash_result
    )
    if(NOT flash_result EQUAL 0)
        message(FATAL_ERROR "Failed to flash ${APP}")
    endif()
endfunction()

# Iterate through each sub-application and build/flash them
foreach(APP_IDX RANGE 0 ${LENGTH_SUB_APP_NAMES}-1)
    list(GET SUB_APP_NAMES ${APP_IDX} APP)
    list(GET SUB_APP_ADDRS ${APP_IDX} ADDR)
    build_and_flash_app(${APP} ${ADDR})
endforeach()
