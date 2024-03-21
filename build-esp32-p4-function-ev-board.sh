#!/usr/bin/env bash

set -e
export BOARD_M5STACK_CORE_S3=none
export BOARD_ESP_BOX_3=none
export BOARD_ESP32_P4_FUNCTION_EV_BOARD=esp32p4
export SDKCONFIG_DEFAULTS=sdkconfig.bsp.esp32_p4_function_ev_board
idf.py --preview set-target esp32p4
idf.py reconfigure build