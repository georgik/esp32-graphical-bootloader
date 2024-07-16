# Paths to binaries
set(BOOTLOADER_BIN "${CMAKE_SOURCE_DIR}/build/bootloader/bootloader.bin")
set(PARTITION_TABLE_BIN "${CMAKE_SOURCE_DIR}/build/partition_table/partition-table.bin")

set(TIC_TAC_TOE_BIN "${CMAKE_SOURCE_DIR}/apps/tic_tac_toe/build/tic_tac_toe.bin")
set(WIFI_LIST_BIN "${CMAKE_SOURCE_DIR}/apps/wifi_list/build/wifi_list.bin")
set(CALCULATOR_BIN "${CMAKE_SOURCE_DIR}/apps/calculator/build/calculator.bin")
set(SYNTH_PIANO_BIN "${CMAKE_SOURCE_DIR}/apps/synth_piano/build/synth_piano.bin")
set(GAME_OF_LIFE_BIN "${CMAKE_SOURCE_DIR}/apps/game_of_life/build/game_of_life.bin")

# Output files
set(COMBINED_BIN "${CMAKE_SOURCE_DIR}/combined.bin")
set(COMBINED_UF2 "${CMAKE_SOURCE_DIR}/combined.uf2")

# Add custom target to merge binaries
add_custom_target(merge_binaries ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Merging binaries into ${COMBINED_BIN}..."
    COMMAND esptool.py --chip esp32s3 merge_bin -o ${COMBINED_BIN}
        --flash_mode dio --flash_freq 80m --flash_size 16MB
        0x1000 ${BOOTLOADER_BIN}
        0x8000 ${PARTITION_TABLE_BIN}
        0x220000 ${TIC_TAC_TOE_BIN}
        0x4E0000 ${WIFI_LIST_BIN}
        0x7A0000 ${CALCULATOR_BIN}
        0xA60000 ${SYNTH_PIANO_BIN}
        0xD20000 ${GAME_OF_LIFE_BIN}
    COMMAND ${CMAKE_COMMAND} -E echo "Converting ${COMBINED_BIN} to ${COMBINED_UF2}..."
    COMMAND python ${CMAKE_SOURCE_DIR}/uf2conv.py ${COMBINED_BIN} --chip esp32s3 --base 0x0 --output ${COMBINED_UF2}
    COMMENT "Merging and converting binaries to UF2"
    VERBATIM
)

# Ensure merge_binaries runs after all binaries are built
add_dependencies(merge_binaries
    tic_tac_toe_app
    wifi_list_app
    calculator_app
    synth_piano_app
    game_of_life_app
)
