cmake_minimum_required(VERSION 3.5)

# Function to select the board
function(select_board)
    if(NOT DEFINED ENV{SDKCONFIG_DEFAULTS})
        message(FATAL_ERROR "Environment variable SDKCONFIG_DEFAULTS is not set.")
    else()
        set(SDKCONFIG_DEFAULTS $ENV{SDKCONFIG_DEFAULTS})
    endif()

    message(STATUS "Using SDKCONFIG_DEFAULTS: ${SDKCONFIG_DEFAULTS}")

    # Map SDKCONFIG_DEFAULTS to the corresponding idf_component.yml template
    if(SDKCONFIG_DEFAULTS STREQUAL "sdkconfig.defaults.esp-box")
        set(IDF_COMPONENT_YML_TEMPLATE "${CMAKE_SOURCE_DIR}/idf_component_templates/esp-box.yml")
    elseif(SDKCONFIG_DEFAULTS STREQUAL "sdkconfig.defaults.esp-box-3")
        set(IDF_COMPONENT_YML_TEMPLATE "${CMAKE_SOURCE_DIR}/idf_component_templates/esp-box-3.yml")
    elseif(SDKCONFIG_DEFAULTS STREQUAL "sdkconfig.defaults.m5stack_core_s3")
        set(IDF_COMPONENT_YML_TEMPLATE "${CMAKE_SOURCE_DIR}/idf_component_templates/m5stack_core_s3.yml")
    elseif(SDKCONFIG_DEFAULTS STREQUAL "sdkconfig.defaults.esp32_p4_function_ev_board")
        set(IDF_COMPONENT_YML_TEMPLATE "${CMAKE_SOURCE_DIR}/idf_component_templates/esp32_p4_function_ev_board.yml")
    else()
        message(FATAL_ERROR "Unsupported SDKCONFIG_DEFAULTS: ${SDKCONFIG_DEFAULTS}")
    endif()

    message(STATUS "IDF_COMPONENT_YML_TEMPLATE: ${IDF_COMPONENT_YML_TEMPLATE}")

    # Copy the appropriate idf_component.yml template to main
    set(IDF_COMPONENT_YML_DEST "${CMAKE_SOURCE_DIR}/main/idf_component.yml")
    message(STATUS "Copying ${IDF_COMPONENT_YML_TEMPLATE} to ${IDF_COMPONENT_YML_DEST}")
    configure_file(${IDF_COMPONENT_YML_TEMPLATE} ${IDF_COMPONENT_YML_DEST} COPYONLY)

    # Verify that the file was copied to main
    if(EXISTS ${IDF_COMPONENT_YML_DEST})
        message(STATUS "File copied successfully to ${IDF_COMPONENT_YML_DEST}")
    else()
        message(FATAL_ERROR "Failed to copy ${IDF_COMPONENT_YML_TEMPLATE} to ${IDF_COMPONENT_YML_DEST}")
    endif()

    # List of sub-applications
    set(SUB_APPS tic_tac_toe wifi_list calculator synth_piano game_of_life)

    # Copy the appropriate idf_component.yml template to each sub-application
    foreach(APP ${SUB_APPS})
        set(IDF_COMPONENT_YML_DEST "${CMAKE_SOURCE_DIR}/apps/${APP}/main/idf_component.yml")
        message(STATUS "Copying ${IDF_COMPONENT_YML_TEMPLATE} to ${IDF_COMPONENT_YML_DEST}")
        configure_file(${IDF_COMPONENT_YML_TEMPLATE} ${IDF_COMPONENT_YML_DEST} COPYONLY)

        # Verify that the file was copied to the sub-application
        if(EXISTS ${IDF_COMPONENT_YML_DEST})
            message(STATUS "File copied successfully to ${IDF_COMPONENT_YML_DEST}")
        else()
            message(FATAL_ERROR "Failed to copy ${IDF_COMPONENT_YML_TEMPLATE} to ${IDF_COMPONENT_YML_DEST}")
        endif()
    endforeach()
endfunction()

# Function to build all applications
function(build_all_apps)
    # Build main app
    message(STATUS "Building main application")
    execute_process(
        COMMAND idf.py build
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
            COMMAND idf.py build
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
