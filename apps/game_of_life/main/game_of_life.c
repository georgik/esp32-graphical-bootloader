#include <stdio.h>
// Conway's Game of Life for ESP32 using LVGL
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_ota_ops.h"

#define TAG "GameOfLife"
#define GRID_SIZE 20
#define CELL_SIZE 10
#define CANVAS_WIDTH  (GRID_SIZE * CELL_SIZE)
#define CANVAS_HEIGHT (GRID_SIZE * CELL_SIZE)

static bool grid[GRID_SIZE][GRID_SIZE];
static bool temp_grid[GRID_SIZE][GRID_SIZE];
static lv_obj_t *canvas;
lv_layer_t layer;

static void draw_grid();
static void update_grid();

static void randomize_grid() {
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            grid[row][col] = rand() % 2;
        }
    }
}

static void reset_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        randomize_grid();
    }
}

static void draw_grid() {
    bsp_display_lock(0);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);

    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            lv_area_t area;
            area.x1 = col * CELL_SIZE;
            area.y1 = row * CELL_SIZE;
            area.x2 = area.x1 + CELL_SIZE - 1;
            area.y2 = area.y1 + CELL_SIZE - 1;
            rect_dsc.bg_color = grid[row][col] ? lv_palette_main(LV_PALETTE_BLUE) : lv_color_white();
            lv_draw_rect(&layer, &rect_dsc, &area);
        }
    }

    lv_canvas_finish_layer(canvas, &layer);
    lv_obj_invalidate(canvas);
    bsp_display_unlock();
}

static void update_grid() {
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            int live_neighbors = 0;
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    if (i == 0 && j == 0) continue;
                    int r = row + i;
                    int c = col + j;
                    if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
                        live_neighbors += grid[r][c];
                    }
                }
            }

            if (grid[row][col]) {
                temp_grid[row][col] = live_neighbors == 2 || live_neighbors == 3;
            } else {
                temp_grid[row][col] = live_neighbors == 3;
            }
        }
    }

    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            grid[row][col] = temp_grid[row][col];
        }
    }
}

static void life_task(void *param) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        update_grid();
        draw_grid();
    }
}

void reset_to_factory_app() {
    // Get the partition structure for the factory partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition != NULL) {
        if (esp_ota_set_boot_partition(factory_partition) == ESP_OK) {
            printf("Set boot partition to factory.\n");
        } else {
            printf("Failed to set boot partition to factory.\n");
        }
    } else {
        printf("Factory partition not found.\n");
    }

    fflush(stdout);
}

void app_main(void) {
    // Reset to factory app for the next boot.
    // It should return to graphical bootloader.
    reset_to_factory_app();

    // Initialize the BSP
    bsp_i2c_init();
    bsp_display_start();
    lv_init();
    srand(time(NULL));

    bsp_display_lock(0);

    LV_DRAW_BUF_DEFINE(draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565);

    canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_draw_buf(canvas, &draw_buf);
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_COVER);
    lv_obj_center(canvas);

    lv_canvas_init_layer(canvas, &layer);

    // Create a reset button
    lv_obj_t *reset_btn = lv_btn_create(lv_scr_act());
    lv_obj_t *label = lv_label_create(reset_btn);
    lv_label_set_text(label, "Reset");
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_obj_add_event_cb(reset_btn, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);

    bsp_display_backlight_on();
    bsp_display_unlock();

    // Initialize grid
    randomize_grid();
    printf("Grid initialized\n");
    draw_grid();
    printf("Grid drawn\n");

    // Create the life task
    xTaskCreate(life_task, "life_task", 32768, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
