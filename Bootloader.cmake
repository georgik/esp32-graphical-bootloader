cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED BUILD_BOARD)
    message(FATAL_ERROR "BUILD_BOARD CMake variable is not set")
endif()

# Function to build all applications
function(build_all_apps)
    # Build main app
    message(STATUS "Building main application")
    execute_process(
        COMMAND idf.py @boards/${BUILD_BOARD}.cfg build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE build_main_result
    )
    if(NOT build_main_result EQUAL 0)
        message(FATAL_ERROR "Failed to build main application")
    endif()

    # List of sub-applications
    set(SUB_APPS tic_tac_toe wifi_list calculator synth_piano game_of_life)

    # Function to build each sub-application
    function(build_app APP)
        message(STATUS "Building ${APP}")
        execute_process(
            COMMAND idf.py @../../boards/${BUILD_BOARD}.cfg build
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/apps/${APP}
            RESULT_VARIABLE build_result
        )
        if(NOT build_result EQUAL 0)
            message(FATAL_ERROR "Failed to build ${APP}")
        endif()
    endfunction()

    # Build each sub-application
    foreach(APP ${SUB_APPS})
        build_app(${APP})
    endforeach()
endfunction()

# Function to merge all binaries into a single .bin file
function(merge_binaries)
    # Paths to binaries
    set(BOOTLOADER_BIN "${CMAKE_SOURCE_DIR}/build/bootloader/bootloader.bin")
    set(PARTITION_TABLE_BIN "${CMAKE_SOURCE_DIR}/build/partition_table/partition-table.bin")
    set(MAIN_APP_BIN "${CMAKE_SOURCE_DIR}/build/esp32-graphical-bootloader.bin")
    set(OTA_DATA_INITIAL_BIN "${CMAKE_SOURCE_DIR}/build/ota_data_initial.bin")

    # List of sub-applications and corresponding flash addresses
    set(SUB_APP_NAMES
        tic_tac_toe
        wifi_list
        calculator
        synth_piano
        game_of_life
    )

    set(SUB_APP_ADDRS
        0x220000
        0x4E0000
        0x7A0000
        0xA60000
        0xD20000
    )

    # Build command for esptool.py merge_bin
    set(MERGE_CMD esptool.py --chip esp32s3 merge_bin -o ${CMAKE_SOURCE_DIR}/build/combined.bin
        --flash_mode dio --flash_size 16MB
        0x0 ${BOOTLOADER_BIN}
        0x8000 ${PARTITION_TABLE_BIN}
        0xf000 ${OTA_DATA_INITIAL_BIN}
        0x20000 ${MAIN_APP_BIN}
    )

    # Append sub-application binaries and addresses
    list(LENGTH SUB_APP_NAMES LENGTH_SUB_APP_NAMES)
    math(EXPR LAST_IDX "${LENGTH_SUB_APP_NAMES} - 1")
    foreach(APP_IDX RANGE 0 ${LAST_IDX})
        list(GET SUB_APP_NAMES ${APP_IDX} APP)
        list(GET SUB_APP_ADDRS ${APP_IDX} ADDR)
        list(APPEND MERGE_CMD ${ADDR} ${CMAKE_SOURCE_DIR}/apps/${APP}/build/${APP}.bin)
    endforeach()

    # Execute merge command
    message(STATUS "Merging binaries into combined.bin...")
    execute_process(
        COMMAND ${MERGE_CMD}
        RESULT_VARIABLE merge_result
    )
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR "Failed to merge binaries")
    endif()
endfunction()

# Function to merge all binaries into a single UF2 file
function(merge_binaries_uf2)
    # Paths to binaries
    set(BOOTLOADER_BIN "${CMAKE_SOURCE_DIR}/build/bootloader/bootloader.bin")
    set(PARTITION_TABLE_BIN "${CMAKE_SOURCE_DIR}/build/partition_table/partition-table.bin")
    set(MAIN_APP_BIN "${CMAKE_SOURCE_DIR}/build/esp32-graphical-bootloader.bin")
    set(OTA_DATA_INITIAL_BIN "${CMAKE_SOURCE_DIR}/build/ota_data_initial.bin")

    # List of sub-applications and corresponding flash addresses
    set(SUB_APP_NAMES
        tic_tac_toe
        wifi_list
        calculator
        synth_piano
        game_of_life
    )

    set(SUB_APP_ADDRS
        0x220000
        0x4E0000
        0x7A0000
        0xA60000
        0xD20000
    )

    # Build command for esptool.py merge_bin with UF2 format
    set(MERGE_CMD esptool.py --chip esp32s3 merge_bin --format uf2 -o ${CMAKE_SOURCE_DIR}/build/uf2.bin
        --flash_mode dio --flash_size 16MB
        0x0 ${BOOTLOADER_BIN}
        0x8000 ${PARTITION_TABLE_BIN}
        0xf000 ${OTA_DATA_INITIAL_BIN}
        0x20000 ${MAIN_APP_BIN}
    )

    # Append sub-application binaries and addresses
    list(LENGTH SUB_APP_NAMES LENGTH_SUB_APP_NAMES)
    math(EXPR LAST_IDX "${LENGTH_SUB_APP_NAMES} - 1")
    foreach(APP_IDX RANGE 0 ${LAST_IDX})
        list(GET SUB_APP_NAMES ${APP_IDX} APP)
        list(GET SUB_APP_ADDRS ${APP_IDX} ADDR)
        list(APPEND MERGE_CMD ${ADDR} ${CMAKE_SOURCE_DIR}/apps/${APP}/build/${APP}.bin)
    endforeach()

    # Execute merge command
    message(STATUS "Merging binaries into uf2.bin...")
    execute_process(
        COMMAND ${MERGE_CMD}
        RESULT_VARIABLE merge_result
    )
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR "Failed to merge binaries into UF2")
    endif()
endfunction()

# Function to run all steps
function(build_all)
    select_board()
    build_all_apps()
    merge_binaries()
endfunction()

# Entry point
if(DEFINED action)
    message(STATUS "Action specified: ${action}")
    if(action STREQUAL "select_board")
        select_board()
    elseif(action STREQUAL "build_all_apps")
        build_all_apps()
    elseif(action STREQUAL "merge_binaries")
        merge_binaries()
    elseif(action STREQUAL "merge_binaries_uf2")
        merge_binaries_uf2()
    elseif(action STREQUAL "build_all")
        build_all()
    else()
        message(FATAL_ERROR "Unknown action: ${action}")
    endif()
else()
    message(FATAL_ERROR "No action specified")
endif()
