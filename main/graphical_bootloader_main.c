/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"

extern void bootloader_ui(lv_obj_t *scr);

void app_main(void)
{
    ESP_LOGI("bootloader", "Starting 3rd stage bootloader...");

    bsp_display_start();

    bsp_display_lock(0);
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    bootloader_ui(scr);

    bsp_display_unlock();
    bsp_display_backlight_on();
     // Enter the main loop to process LVGL tasks
    while (1) {
        // Handle LVGL related tasks
        lv_task_handler();
        // Delay for a short period of time (e.g., 5 ms)
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
