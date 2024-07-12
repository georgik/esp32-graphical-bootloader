#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_ota_ops.h"

#define TAG "TicTacToe"

static lv_obj_t *btn_grid[3][3];
static char board[3][3];
static bool player_turn = true; // true for 'X', false for 'O'

static void reset_game() {
    bsp_display_lock(0);
    memset(board, 0, sizeof(board));
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            lv_obj_t *btn = btn_grid[row][col];
            lv_label_set_text(lv_obj_get_child(btn, 0), "");
            lv_obj_clear_state(btn, LV_STATE_DISABLED);
        }
    }
    player_turn = true;
    bsp_display_unlock();
}

static void close_message_box(lv_timer_t *timer) {
    lv_obj_t *msg_box = (lv_obj_t *)lv_timer_get_user_data(timer);
    bsp_display_lock(0);
    lv_msgbox_close(msg_box);
    lv_timer_delete(timer);
    bsp_display_unlock();
    reset_game();
}

static void show_message(const char *text) {
    ESP_LOGI(TAG, "%s", text);
    bsp_display_lock(0);
    lv_obj_t *msg_box = lv_msgbox_create(NULL);
    lv_msgbox_add_text(msg_box, text);
    lv_obj_center(msg_box);
    lv_timer_create(close_message_box, 2000, msg_box);
    bsp_display_unlock();
}

static bool check_winner(char player) {
    for (int i = 0; i < 3; ++i) {
        // Check rows
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
            return true;
        // Check columns
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
            return true;
    }
    // Check diagonals
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
        return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
        return true;
    return false;
}

static void event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        bsp_display_lock(0);
        uint32_t id = (uint32_t)lv_event_get_user_data(e);
        uint8_t row = id / 3;
        uint8_t col = id % 3;
        if (board[row][col] == 0) {
            board[row][col] = player_turn ? 'X' : 'O';
            lv_label_set_text(lv_obj_get_child(btn, 0), player_turn ? "X" : "O");
            lv_obj_add_state(btn, LV_STATE_DISABLED); // Disable the button after being clicked

            if (check_winner(player_turn ? 'X' : 'O')) {
                show_message(player_turn ? "Player X wins!" : "Player O wins!");
            } else if (memchr(board, 0, sizeof(board)) == NULL) {
                show_message("It's a draw!");
            } else {
                player_turn = !player_turn;
            }
        }
        bsp_display_unlock();
    }
}

void lvgl_task(void *pvParameter) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Add a small delay to prevent watchdog issues
    }
}

void reset_to_factory_app() {
    // Get the partition structure for the factory partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition != NULL) {
        if (esp_ota_set_boot_partition(factory_partition) == ESP_OK) {
            printf("Set boot partition to factory, restarting now.\n");
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

    // Create a grid for buttons
    lv_obj_t *grid = lv_obj_create(lv_scr_act());
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_size(grid, 240, 240);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 0);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            lv_obj_t *btn = lv_btn_create(grid);
            lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, "");
            lv_obj_center(label);
            lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, (void *)(row * 3 + col));
            btn_grid[row][col] = btn;
        }
    }

    bsp_display_backlight_on();

    reset_game();

    // Create a task to handle LVGL
    xTaskCreate(lvgl_task, "lvgl_task", 16384, NULL, 1, NULL);
}
