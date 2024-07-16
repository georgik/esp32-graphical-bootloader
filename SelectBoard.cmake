# Check if SDKCONFIG_DEFAULTS is set
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
set(SUB_APPS calculator game_of_life synth_piano tic_tac_toe wifi_list)

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
